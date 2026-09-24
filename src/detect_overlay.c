#include "detect_overlay.h"

#include "module_flags.h"

/*
 * The ERR line is intentionally not gated by debug_level: it is the only
 * always-on view of what the overlay will draw, and it lets a field report
 * distinguish "backend produced no boxes" from "browser failed to draw them".
 * Throttled to one line per second, plus every target-count transition.
 */
#define CA_DETECT_OVERLAY_LOG_INTERVAL_MS 1000

static pthread_mutex_t g_snapshot_mutex;
static CaDetSnapshot g_snapshot;
static unsigned long long g_publish_seq;
static int g_inited;
static int g_warned_no_init;
static int g_last_logged_count = -1;
static int64_t g_last_log_ms;

static void log_snapshot(int raw_count, const CaDetSnapshot *snap)
{
    int64_t now = ca_now_ms();

    if (snap->count == g_last_logged_count &&
        now - g_last_log_ms < CA_DETECT_OVERLAY_LOG_INTERVAL_MS) {
        return;
    }
    g_last_logged_count = snap->count;
    g_last_log_ms = now;

    if (snap->count <= 0) {
        ca_log("ERR", "detect overlay: n=0 frame=%dx%d ts=%lld",
               snap->frame_w, snap->frame_h, (long long)snap->ts_ms);
        return;
    }

    ca_log("ERR",
           "detect overlay: n=%d%s frame=%dx%d ts=%lld box0=(%.4f,%.4f,%.4f,%.4f) cls=%d conf=%.2f id=%d name=%s",
           snap->count, raw_count > snap->count ? " truncated" : "",
           snap->frame_w, snap->frame_h, (long long)snap->ts_ms,
           (double)snap->boxes[0].x, (double)snap->boxes[0].y,
           (double)snap->boxes[0].w, (double)snap->boxes[0].h,
           snap->boxes[0].cls, (double)snap->boxes[0].confidence,
           snap->boxes[0].id, snap->boxes[0].name);
}

static float clamp_unit(float v)
{
    if (!(v > 0.0f)) {
        return 0.0f; /* also folds NaN and negatives into 0 */
    }
    return v > 1.0f ? 1.0f : v;
}

int ca_detect_overlay_init(void)
{
    if (g_inited) {
        return CA_OK;
    }
    if (pthread_mutex_init(&g_snapshot_mutex, NULL) != 0) {
        ca_log("ERR", "detect overlay: mutex init failed");
        return CA_ERR;
    }
    g_inited = 1;
    return CA_OK;
}

int ca_detect_overlay_publish(const CaDetBox *boxes, int count, int frame_w,
                              int frame_h, int64_t ts_ms)
{
#if !CA_ENABLE_DETECT_OVERLAY
    (void)boxes;
    (void)count;
    (void)frame_w;
    (void)frame_h;
    (void)ts_ms;
    return CA_OK;
#else
    CaDetSnapshot next;
    int raw_count = count;
    int i;

    if (count < 0 || (count > 0 && boxes == NULL)) {
        return CA_ERR;
    }
    if (count > CA_DETECT_OVERLAY_MAX_BOXES) {
        count = CA_DETECT_OVERLAY_MAX_BOXES;
    }
    if (!g_inited) {
        if (!g_warned_no_init) {
            g_warned_no_init = 1;
            ca_log("ERR", "detect overlay: publish before init, dropped");
        }
        return CA_ERR;
    }

    memset(&next, 0, sizeof(next));
    next.ts_ms = ts_ms;
    next.frame_w = frame_w > 0 ? frame_w : 0;
    next.frame_h = frame_h > 0 ? frame_h : 0;
    next.count = count;
    for (i = 0; i < count; ++i) {
        next.boxes[i].x = clamp_unit(boxes[i].x);
        next.boxes[i].y = clamp_unit(boxes[i].y);
        next.boxes[i].w = clamp_unit(boxes[i].w);
        next.boxes[i].h = clamp_unit(boxes[i].h);
        next.boxes[i].cls = boxes[i].cls;
        next.boxes[i].confidence = clamp_unit(boxes[i].confidence);
        next.boxes[i].id = boxes[i].id;
        memcpy(next.boxes[i].name, boxes[i].name, sizeof(next.boxes[i].name));
        next.boxes[i].name[CA_DETECT_OVERLAY_NAME_LEN - 1] = '\0';
    }

    pthread_mutex_lock(&g_snapshot_mutex);
    next.seq = ++g_publish_seq;
    g_snapshot = next;
    pthread_mutex_unlock(&g_snapshot_mutex);

    log_snapshot(raw_count, &next);
    return CA_OK;
#endif
}

void ca_detect_overlay_get(CaDetSnapshot *out)
{
    if (out == NULL) {
        return;
    }
    memset(out, 0, sizeof(*out));
    if (!g_inited) {
        return;
    }
    pthread_mutex_lock(&g_snapshot_mutex);
    *out = g_snapshot;
    pthread_mutex_unlock(&g_snapshot_mutex);
}
