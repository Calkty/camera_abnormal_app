#include "clip_writer.h"
#include "config.h"
#include "event_queue.h"
#include "infer_adapter.h"
#include "ring_buffer.h"
#include "rtsp_client.h"
#include "uploader.h"
#include "module_flags.h"

#include <pthread.h>
#include <signal.h>
#include <stdlib.h>

static volatile int g_running = 1;

/* Independent of RTSP progress and debug_level; never copies packet payloads. */
static void ring_diag(PacketRing *rb)
{
    FILE *fp;
    char line[256];
    long rss = -1, hwm = -1, vm = -1;
    fp = fopen("/proc/self/status", "r");
    if (fp) {
        while (fgets(line, sizeof(line), fp)) {
            if (sscanf(line, "VmRSS: %ld", &rss) == 1) continue;
            if (sscanf(line, "VmHWM: %ld", &hwm) == 1) continue;
            if (sscanf(line, "VmSize: %ld", &vm) == 1) continue;
        }
        fclose(fp);
    }
    ca_log("INFO", "DIAG memory: rss_kb=%ld peak_rss_kb=%ld vm_kb=%ld", rss, hwm, vm);
    if (rb) {
        unsigned long long bytes = 0;
        int i, count, cap, max_nal = 0;
        int64_t oldest = 0, newest = 0;
        size_t limit, peak;
        unsigned long long by_time, by_bytes, by_count, oversize, failures;
        if (pthread_mutex_trylock(&rb->mutex) != 0) {
            ca_log("INFO", "DIAG ring: lock_busy");
            return;
        }
        count = rb->count;
        cap = rb->capacity;
        limit = rb->max_bytes;
        peak = rb->peak_payload_bytes;
        by_time = rb->evicted_time;
        by_bytes = rb->evicted_bytes;
        by_count = rb->evicted_count;
        oversize = rb->dropped_oversize;
        failures = rb->alloc_failures;
        for (i = 0; i < count; ++i) {
            EncodedPacket *p = &rb->pkts[(rb->head + i) % cap];
            if (!p->data || p->size <= 0) continue;
            bytes += (unsigned long long)p->size;
            if (p->size > max_nal) max_nal = p->size;
            if (!oldest) oldest = p->recv_ms;
            newest = p->recv_ms;
        }
        pthread_mutex_unlock(&rb->mutex);
        ca_log("INFO", "DIAG ring: count=%d capacity=%d payload_bytes=%llu span_ms=%lld newest_age_ms=%lld max_nal_bytes=%d",
               count, cap, bytes, (long long)(newest - oldest),
               newest ? (long long)(ca_now_ms() - newest) : -1LL, max_nal);
        ca_log("INFO", "DIAG ring limits: max_bytes=%llu peak_bytes=%llu evict_time=%llu evict_bytes=%llu evict_count=%llu oversize=%llu malloc_fail=%llu",
               (unsigned long long)limit, (unsigned long long)peak,
               by_time, by_bytes, by_count, oversize, failures);
    }
}

static void on_signal(int sig)
{
    (void)sig;
    g_running = 0;
}

static int ring_capacity_from_config(const AppConfig *cfg)
{
    (void)cfg;
    /* Metadata slots only; retention is controlled by time AND payload bytes. */
    return 4096;
}

int main(int argc, char **argv)
{
    const char *conf = argc > 1 ? argv[1] : "app.conf";
    AppConfig cfg;
    PacketRing ring;
    EventQueue event_queue;
    UploadQueue upload_queue;
    pthread_t tid_infer = 0;
    pthread_t tid_rtsp = 0;
    pthread_t tid_clip = 0;
    pthread_t tid_upload = 0;
    InferContext infer_ctx;
    RtspRecordContext rtsp_ctx;
    ClipWriterContext clip_ctx;
    UploadContext upload_ctx;
    int rc = 0;
    int ring_ready = 0, event_ready = 0, upload_ready = 0;

    signal(SIGINT, on_signal);
    signal(SIGTERM, on_signal);

    if (config_load(conf, &cfg) != CA_OK) {
        return 1;
    }
    ca_set_debug_level(cfg.debug_level);
    ca_log("INFO", "modules: infer=%d ring_rtsp=%d clip=%d upload=%d legacy_alarm=%d",
           CA_ENABLE_INFER, CA_ENABLE_RING, CA_ENABLE_CLIP,
           CA_ENABLE_UPLOAD, CA_ENABLE_INFER && CA_ENABLE_LEGACY_ALARM);
    ca_log("INFO", "camera_event_app start camera_id=%s rtsp=%s upload=%s",
           cfg.camera_id, cfg.rtsp_url, cfg.upload_url);
    ca_debug_log(1, "config: pre=%ds post=%ds ring=%ds fps=%d max_events=%d codec=%d infer_interval=%ds model=%s abnormal_classes=%s",
                 cfg.pre_seconds, cfg.post_seconds, cfg.ring_seconds, cfg.fps, cfg.max_events,
                 cfg.codec, cfg.infer_interval_seconds,
                 cfg.hikflow_model_path[0] ? cfg.hikflow_model_path : "(hikflow_config.json)",
                 cfg.abnormal_classes[0] ? cfg.abnormal_classes : "(sel_class fallback)");

    if (CA_ENABLE_CLIP) {
        if (ca_prepare_work_dir(&cfg) != CA_OK) {
            ca_log("ERR", "clip output dir unusable; clips will fail until work_dir is fixed");
        }
    }

    if (CA_ENABLE_RING) {
        if (ring_init(&ring, ring_capacity_from_config(&cfg), cfg.ring_seconds,
                      (size_t)cfg.ring_max_mb * 1024 * 1024) != CA_OK)
            goto init_failed;
        ring_ready = 1;
    }
    if (CA_ENABLE_CLIP) {
        if (event_queue_init(&event_queue, cfg.max_events) != CA_OK)
            goto init_failed;
        event_ready = 1;
    }
    if (CA_ENABLE_UPLOAD) {
        if (upload_queue_init(&upload_queue, cfg.max_events) != CA_OK)
            goto init_failed;
        upload_ready = 1;
    }
    ca_debug_log(1, "runtime buffers: ring_capacity=%d event_queue=%d upload_queue=%d",
                 ring_ready ? ring_capacity_from_config(&cfg) : 0,
                 event_ready ? cfg.max_events : 0, upload_ready ? cfg.max_events : 0);

    infer_ctx.cfg = &cfg;
    infer_ctx.event_queue = event_ready ? &event_queue : NULL;
    infer_ctx.running = &g_running;
    rtsp_ctx.cfg = &cfg;
    rtsp_ctx.ring = &ring;
    rtsp_ctx.running = &g_running;
    clip_ctx.cfg = &cfg;
    clip_ctx.ring = &ring;
    clip_ctx.event_queue = &event_queue;
    clip_ctx.upload_queue = upload_ready ? &upload_queue : NULL;
    clip_ctx.running = &g_running;
    upload_ctx.cfg = &cfg;
    upload_ctx.queue = &upload_queue;
    upload_ctx.running = &g_running;

    if ((CA_ENABLE_RING && pthread_create(&tid_rtsp, NULL, rtsp_record_thread, &rtsp_ctx) != 0) ||
        ((CA_ENABLE_INFER || CA_ENABLE_CLIP) && pthread_create(&tid_infer, NULL, infer_thread, &infer_ctx) != 0) ||
        (CA_ENABLE_CLIP && pthread_create(&tid_clip, NULL, event_clip_thread, &clip_ctx) != 0) ||
        (CA_ENABLE_UPLOAD && pthread_create(&tid_upload, NULL, upload_thread, &upload_ctx) != 0)) {
        ca_log("ERR", "pthread_create failed");
        g_running = 0;
        rc = 1;
    }

    ca_log("INFO", "DIAG build=ring-fix-v2 discard=%s ring_seconds=%d fps=%d debug=%d",
           getenv("CA_DIAG_RING_DISCARD") ? getenv("CA_DIAG_RING_DISCARD") : "0",
           cfg.ring_seconds, cfg.fps, cfg.debug_level);
    if (CA_ENABLE_CLIP && (int64_t)cfg.ring_seconds < (int64_t)cfg.pre_seconds + cfg.post_seconds + 2)
        ca_log("WARN", "ring retention is short for queued clips; video may be incomplete (pre=%d post=%d ring=%d)",
               cfg.pre_seconds, cfg.post_seconds, cfg.ring_seconds);
    {
        int ticks = 0;
        while (g_running) {
            if (ring_ready) ring_prune(&ring); /* Also expire data when RTSP stops. */
            if (ticks++ % 10 == 0) ring_diag(ring_ready ? &ring : NULL);
            ca_sleep_ms(500);
        }
    }

    ca_log("INFO", "stopping");
    if (event_ready) event_queue_close(&event_queue);
    if (upload_ready) upload_queue_close(&upload_queue);
    if (tid_infer) {
        pthread_join(tid_infer, NULL);
    }
    if (tid_clip) {
        pthread_join(tid_clip, NULL);
    }
    if (tid_upload) {
        pthread_join(tid_upload, NULL);
    }
    if (tid_rtsp) {
        pthread_join(tid_rtsp, NULL);
    }

cleanup:
    if (upload_ready) upload_queue_destroy(&upload_queue);
    if (event_ready) event_queue_destroy(&event_queue);
    if (ring_ready) ring_destroy(&ring);
    return rc;
init_failed:
    ca_log("ERR", "init queues failed");
    rc = 1;
    goto cleanup;
}
