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

/* Published by main() right after config_load(). Uses its own lock with a
 * static initializer: ca_detect_overlay_init() may not have run yet here, and
 * the ISAPI thread reads this while the algorithm thread is already running. */
static pthread_mutex_t g_url_mutex = PTHREAD_MUTEX_INITIALIZER;
static char g_record_url[CA_MAX_URL];

static void copy_url(char *dst, size_t dst_len, const char *src)
{
    size_t n;

    if (dst_len == 0) {
        return;
    }
    if (src == NULL) {
        dst[0] = '\0';
        return;
    }
    n = strlen(src);
    if (n >= dst_len) {
        n = dst_len - 1;
    }
    memcpy(dst, src, n);
    dst[n] = '\0';
}
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
           "detect overlay: n=%d%s frame=%dx%d ts=%lld box0=(%.4f,%.4f,%.4f,%.4f) cls=%d id=%d name=%s",
           snap->count, raw_count > snap->count ? " truncated" : "",
           snap->frame_w, snap->frame_h, (long long)snap->ts_ms,
           (double)snap->boxes[0].x, (double)snap->boxes[0].y,
           (double)snap->boxes[0].w, (double)snap->boxes[0].h,
           snap->boxes[0].cls, snap->boxes[0].id, snap->boxes[0].name);
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

void ca_detect_overlay_set_upload_url(const char *upload_url)
{
    char buf[CA_MAX_URL];
    char derived[CA_MAX_URL];
    char *scheme;
    char *slash;
    size_t len;
    size_t keep;

    copy_url(buf, sizeof(buf), upload_url);
    /* app.conf is a text file, so the value can carry trailing CR/LF/blanks. */
    len = strlen(buf);
    while (len > 0 && (buf[len - 1] == ' ' || buf[len - 1] == '\t' ||
                       buf[len - 1] == '\r' || buf[len - 1] == '\n')) {
        buf[--len] = '\0';
    }

    derived[0] = '\0';
    scheme = strstr(buf, "://");
    slash = strrchr(buf, '/');
    /* Needs a scheme and a path segment after the authority:
       http://host:port/api/upload -> http://host:port/api/record */
    if (scheme != NULL && slash != NULL && slash > scheme + 3) {
        keep = (size_t)(slash - buf) + 1;
        if (slash[1] == '\0') {
            keep = len; /* URL ends with '/', so just append the leaf */
        }
        if (keep + strlen("record") < sizeof(derived)) {
            memcpy(derived, buf, keep);
            memcpy(derived + keep, "record", strlen("record") + 1);
        }
    }

    pthread_mutex_lock(&g_url_mutex);
    copy_url(g_record_url, sizeof(g_record_url), derived);
    pthread_mutex_unlock(&g_url_mutex);

    if (derived[0] == '\0') {
        ca_log("ERR", "detect overlay: upload_url '%s' unusable, web record keeps its default",
               buf);
    } else {
        ca_log("INFO", "web record endpoint: %s", derived);
    }
}

int ca_detect_overlay_get_record_url(char *out, size_t out_len)
{
    if (out == NULL || out_len == 0) {
        return CA_ERR;
    }
    out[0] = '\0';
    pthread_mutex_lock(&g_url_mutex);
    copy_url(out, out_len, g_record_url);
    pthread_mutex_unlock(&g_url_mutex);
    return out[0] != '\0' ? CA_OK : CA_ERR;
}
