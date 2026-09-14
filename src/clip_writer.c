#include "clip_writer.h"

#include <stdlib.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <unistd.h>

static int mkdir_if_needed(const char *path)
{
    struct stat st;

    if (mkdir(path, 0755) == 0) {
        return CA_OK;
    }
    if (errno == EEXIST) {
        if (stat(path, &st) != 0) {
            ca_log("ERR", "path exists but stat failed: %s errno=%d (%s)",
                   path, errno, strerror(errno));
            return CA_ERR;
        }
        if (!S_ISDIR(st.st_mode)) {
            ca_log("ERR", "path exists but is not a directory: %s", path);
            return CA_ERR;
        }
        return CA_OK;
    }
    ca_log("ERR", "mkdir failed: %s errno=%d (%s)", path, errno, strerror(errno));
    return CA_ERR;
}

/* Create the clip output tree early, so a broken work_dir shows up at startup
 * instead of only at the first event. */
int ca_prepare_work_dir(const AppConfig *cfg)
{
    struct statvfs vfs;
    char event_dir[CA_MAX_FULL_PATH];

    if (!cfg) {
        return CA_ERR;
    }
    if (snprintf(event_dir, sizeof(event_dir), "%s/events", cfg->work_dir) >=
        (int)sizeof(event_dir)) {
        ca_log("ERR", "event dir path too long: work_dir=%s", cfg->work_dir);
        return CA_ERR;
    }
    if (mkdir_if_needed(cfg->work_dir) != CA_OK) {
        ca_log("ERR", "work_dir unusable: %s (read-only? no space? not a directory?)",
               cfg->work_dir);
        return CA_ERR;
    }
    if (statvfs(cfg->work_dir, &vfs) == 0) {
        unsigned long long free_mb =
            (unsigned long long)vfs.f_bavail * (unsigned long long)vfs.f_frsize / (1024ULL * 1024ULL);
        ca_log("INFO", "work_dir %s free space: %llu MB", cfg->work_dir, free_mb);
    } else {
        ca_log("WARN", "statvfs failed: %s errno=%d (%s)",
               cfg->work_dir, errno, strerror(errno));
    }
    if (mkdir_if_needed(event_dir) != CA_OK) {
        ca_log("ERR", "events dir unusable: %s", event_dir);
        return CA_ERR;
    }
    ca_log("INFO", "event output dir ready: %s", event_dir);
    return CA_OK;
}

static int append_packet(EncodedPacket **arr, int *count, int *cap, const EncodedPacket *pkt)
{
    EncodedPacket *n;
    if (*count >= *cap) {
        int new_cap = *cap > 0 ? (*cap * 2) : 1024;
        n = (EncodedPacket *)realloc(*arr, (size_t)new_cap * sizeof(EncodedPacket));
        if (!n) {
            return CA_ERR;
        }
        memset(n + *cap, 0, (size_t)(new_cap - *cap) * sizeof(EncodedPacket));
        *arr = n;
        *cap = new_cap;
    }
    (*arr)[*count] = *pkt;
    (*arr)[*count].data = (uint8_t *)malloc((size_t)pkt->size);
    if (!(*arr)[*count].data) {
        return CA_ERR;
    }
    memcpy((*arr)[*count].data, pkt->data, (size_t)pkt->size);
    (*count)++;
    return CA_OK;
}

static int append_range(EncodedPacket **clip, int *clip_count, int *clip_cap,
                        EncodedPacket *snap, int snap_count, int64_t start_ms,
                        int64_t end_ms, int64_t min_recv_ms)
{
    int i;
    for (i = 0; i < snap_count; i++) {
        if (snap[i].recv_ms < start_ms || snap[i].recv_ms > end_ms || snap[i].recv_ms <= min_recv_ms) {
            continue;
        }
        if (append_packet(clip, clip_count, clip_cap, &snap[i]) != CA_OK) {
            return CA_ERR;
        }
    }
    return CA_OK;
}

static int align_to_keyframe(EncodedPacket *clip, int count, int64_t desired_start)
{
    int i;
    int best_before = -1;
    int first_after = -1;
    for (i = 0; i < count; i++) {
        if (clip[i].key_frame && clip[i].recv_ms <= desired_start) {
            best_before = i;
        }
        if (first_after < 0 && clip[i].key_frame && clip[i].recv_ms > desired_start) {
            first_after = i;
        }
    }
    if (best_before >= 0) {
        return best_before;
    }
    if (first_after >= 0) {
        return first_after;
    }
    return 0;
}

static int write_json(const char *path, const AppConfig *cfg, const AbnormalEvent *ev,
                      CodecType codec, const char *video_path, int packet_count,
                      int64_t clip_start_ms, int64_t clip_end_ms)
{
    FILE *fp = fopen(path, "wb");
    int written;
    if (!fp) {
        ca_log("ERR", "write_json open failed: %s errno=%d (%s)",
               path, errno, strerror(errno));
        return CA_ERR;
    }
    written = fprintf(fp,
            "{\n"
            "  \"camera_id\": \"%s\",\n"
            "  \"event_type\": \"%s\",\n"
            "  \"confidence\": %.4f,\n"
            "  \"event_wall_ms\": %lld,\n"
            "  \"detect_done_ms\": %lld,\n"
            "  \"clip_start_ms\": %lld,\n"
            "  \"clip_end_ms\": %lld,\n"
            "  \"codec\": \"%s\",\n"
            "  \"fps\": %d,\n"
            "  \"packet_count\": %d,\n"
            "  \"video_path\": \"%s\",\n"
            "  \"server_should_convert_mp4\": true\n"
            "}\n",
            cfg->camera_id, ev->event_type, ev->confidence,
            (long long)ev->event_wall_ms, (long long)ev->detect_done_ms,
            (long long)clip_start_ms, (long long)clip_end_ms,
            codec == CODEC_H265 ? "h265" : "h264", cfg->fps, packet_count, video_path);
    if (written < 0 || fflush(fp) != 0) {
        ca_log("ERR", "write_json write failed: %s errno=%d (%s)",
               path, errno, strerror(errno));
        fclose(fp);
        return CA_ERR;
    }
    if (fclose(fp) != 0) {
        ca_log("ERR", "write_json close failed: %s errno=%d (%s)",
               path, errno, strerror(errno));
        return CA_ERR;
    }
    return CA_OK;
}

static int write_clip_with_params(PacketRing *ring, const char *video_path, const char *meta_path,
                                  const AppConfig *cfg, const AbnormalEvent *ev,
                                  EncodedPacket *clip, int clip_count, int start_idx)
{
    FILE *fp;
    int i;
    CodecType codec;
    EncodedPacket *params = NULL;
    int param_count = 0;
    int64_t start_ms;
    int64_t end_ms;
    if (clip_count <= 0 || start_idx >= clip_count) {
        ca_log("ERR", "write clip skipped: clip_count=%d start_idx=%d", clip_count, start_idx);
        return CA_ERR;
    }
    codec = clip[start_idx].codec == CODEC_UNKNOWN ? cfg->codec : clip[start_idx].codec;
    fp = fopen(video_path, "wb");
    if (!fp) {
        ca_log("ERR", "open clip video failed: %s errno=%d (%s)",
               video_path, errno, strerror(errno));
        return CA_ERR;
    }
    if (ring_get_param_sets(ring, codec, &params, &param_count) == CA_OK) {
        for (i = 0; i < param_count; i++) {
            if (fwrite(params[i].data, 1, (size_t)params[i].size, fp) != (size_t)params[i].size) {
                ca_log("ERR", "write param set failed: %s index=%d size=%d errno=%d (%s)",
                       video_path, i, params[i].size, errno, strerror(errno));
                packet_array_free(params, param_count);
                fclose(fp);
                return CA_ERR;
            }
        }
        packet_array_free(params, param_count);
    } else {
        ca_log("WARN", "clip param sets unavailable: video=%s codec=%s",
               video_path, codec == CODEC_H265 ? "h265" : "h264");
    }
    start_ms = clip[start_idx].recv_ms;
    end_ms = clip[clip_count - 1].recv_ms;
    for (i = start_idx; i < clip_count; i++) {
        if (fwrite(clip[i].data, 1, (size_t)clip[i].size, fp) != (size_t)clip[i].size) {
            ca_log("ERR", "write clip packet failed: %s index=%d size=%d errno=%d (%s)",
                   video_path, i, clip[i].size, errno, strerror(errno));
            fclose(fp);
            return CA_ERR;
        }
    }
    if (fclose(fp) != 0) {
        ca_log("ERR", "close clip video failed: %s errno=%d (%s)",
               video_path, errno, strerror(errno));
        return CA_ERR;
    }
    return write_json(meta_path, cfg, ev, codec, video_path, clip_count - start_idx, start_ms, end_ms);
}

static int build_clip(ClipWriterContext *ctx, const AbnormalEvent *ev, UploadJob *job)
{
    EncodedPacket *clip = NULL;
    int clip_count = 0;
    int clip_cap = 0;
    EncodedPacket *snap = NULL;
    int snap_count = 0;
    int64_t start_ms = ev->event_wall_ms - (int64_t)ctx->cfg->pre_seconds * 1000;
    int64_t end_ms = ev->event_wall_ms + (int64_t)ctx->cfg->post_seconds * 1000;
    int64_t last_recv = 0;
    int start_idx;
    char event_dir[CA_MAX_FULL_PATH];
    char base[CA_MAX_FULL_PATH];
    const char *ext;
    CodecType codec = ctx->cfg->codec;

    if (snprintf(event_dir, sizeof(event_dir), "%s/events", ctx->cfg->work_dir) >=
        (int)sizeof(event_dir)) {
        ca_log("ERR", "event dir path too long: work_dir=%s", ctx->cfg->work_dir);
        packet_array_free(clip, clip_count);
        return CA_ERR;
    }
    ca_debug_log(1, "clip window: event_ms=%lld start_ms=%lld end_ms=%lld pre=%d post=%d",
                 (long long)ev->event_wall_ms, (long long)start_ms, (long long)end_ms,
                 ctx->cfg->pre_seconds, ctx->cfg->post_seconds);
    if (mkdir_if_needed(ctx->cfg->work_dir) != CA_OK) {
        ca_log("ERR", "clip aborted: work_dir unusable: %s (read-only? no space? not a directory?)",
               ctx->cfg->work_dir);
        packet_array_free(clip, clip_count);
        return CA_ERR;
    }
    if (mkdir_if_needed(event_dir) != CA_OK) {
        ca_log("ERR", "clip aborted: events dir unusable: %s", event_dir);
        packet_array_free(clip, clip_count);
        return CA_ERR;
    }

    if (ring_snapshot(ctx->ring, &snap, &snap_count) == CA_OK) {
        if (append_range(&clip, &clip_count, &clip_cap, snap, snap_count, start_ms,
                         ev->event_wall_ms, -1) != CA_OK) {
            ca_log("WARN", "clip pre-range append failed: snapshot=%d appended=%d",
                   snap_count, clip_count);
        }
        ca_debug_log(1, "clip pre-range: snapshot=%d appended=%d", snap_count, clip_count);
        if (clip_count > 0) {
            last_recv = clip[clip_count - 1].recv_ms;
            codec = clip[clip_count - 1].codec;
        }
        packet_array_free(snap, snap_count);
    } else {
        ca_log("WARN", "clip pre-range snapshot failed");
    }
    while (*ctx->running && ca_now_ms() < end_ms + 300) {
        ca_sleep_ms(100);
    }
    snap = NULL;
    snap_count = 0;
    if (ring_snapshot(ctx->ring, &snap, &snap_count) == CA_OK) {
        if (append_range(&clip, &clip_count, &clip_cap, snap, snap_count, ev->event_wall_ms,
                         end_ms, last_recv) != CA_OK) {
            ca_log("WARN", "clip post-range append failed: snapshot=%d total_appended=%d",
                   snap_count, clip_count);
        }
        ca_debug_log(1, "clip post-range: snapshot=%d total_appended=%d last_recv=%lld",
                     snap_count, clip_count, (long long)last_recv);
        if (clip_count > 0) {
            codec = clip[clip_count - 1].codec;
        }
        packet_array_free(snap, snap_count);
    } else {
        ca_log("WARN", "clip post-range snapshot failed");
    }
    if (clip_count <= 0) {
        ca_log("ERR", "clip empty: no packets in window event_ms=%lld start_ms=%lld end_ms=%lld",
               (long long)ev->event_wall_ms, (long long)start_ms, (long long)end_ms);
        packet_array_free(clip, clip_count);
        return CA_ERR;
    }
    start_idx = align_to_keyframe(clip, clip_count, start_ms);
    ca_debug_log(1, "clip align: total_packets=%d start_idx=%d codec=%s",
                 clip_count, start_idx, codec == CODEC_H265 ? "h265" : "h264");
    ext = codec == CODEC_H265 ? "h265" : "h264";
    if (snprintf(base, sizeof(base), "%s/%s_%lld", event_dir, ctx->cfg->camera_id,
                 (long long)ev->event_wall_ms) >= (int)sizeof(base) ||
        snprintf(job->video_path, sizeof(job->video_path), "%s.%s", base, ext) >=
            (int)sizeof(job->video_path) ||
        snprintf(job->meta_path, sizeof(job->meta_path), "%s.json", base) >=
            (int)sizeof(job->meta_path)) {
        ca_log("ERR", "clip output path too long: event_dir=%s", event_dir);
        packet_array_free(clip, clip_count);
        return CA_ERR;
    }
    snprintf(job->camera_id, sizeof(job->camera_id), "%s", ctx->cfg->camera_id);
    snprintf(job->event_type, sizeof(job->event_type), "%s", ev->event_type);
    job->event_wall_ms = ev->event_wall_ms;
    job->codec = codec;
    if (write_clip_with_params(ctx->ring, job->video_path, job->meta_path, ctx->cfg, ev,
                               clip, clip_count, start_idx) != CA_OK) {
        ca_log("ERR", "clip write failed: video=%s meta=%s packets=%d start_idx=%d codec=%s",
               job->video_path, job->meta_path, clip_count, start_idx, ext);
        packet_array_free(clip, clip_count);
        return CA_ERR;
    }
    packet_array_free(clip, clip_count);
    return CA_OK;
}

void *event_clip_thread(void *arg)
{
    ClipWriterContext *ctx = (ClipWriterContext *)arg;
    AbnormalEvent ev;
    while (*ctx->running && event_queue_pop(ctx->event_queue, &ev) == CA_OK) {
        UploadJob job;
        memset(&job, 0, sizeof(job));
        ca_log("INFO", "event received: type=%s confidence=%.3f", ev.event_type, ev.confidence);
        if (build_clip(ctx, &ev, &job) == CA_OK) {
            ca_log("INFO", "clip ready: %s", job.video_path);
            if (!ctx->upload_queue) {
                ca_log("INFO", "upload disabled; clip retained locally: %s", job.video_path);
            } else if (upload_queue_push(ctx->upload_queue, &job, 1) != CA_OK) {
                ca_log("ERR", "upload queue push failed: %s", job.video_path);
            }
        } else {
            ca_log("ERR", "clip build failed for event %lld work_dir=%s (reason logged above)",
                   (long long)ev.event_wall_ms, ctx->cfg->work_dir);
        }
    }
    return NULL;
}
