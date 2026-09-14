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

static void on_signal(int sig)
{
    (void)sig;
    g_running = 0;
}

static int ring_capacity_from_config(const AppConfig *cfg)
{
    int cap = cfg->ring_seconds * cfg->fps * 12;
    if (cap < 8192) {
        cap = 8192;
    }
    return cap;
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
        if (ring_init(&ring, ring_capacity_from_config(&cfg), cfg.ring_seconds) != CA_OK)
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

    while (g_running) {
        ca_sleep_ms(500);
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
