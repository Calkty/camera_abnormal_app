/* Compile with tests/compat only for Windows local tests, not production. */
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

int64_t test_now_ms = 1000;
int g_ca_debug_level = 0;

#include "../src/detect_overlay.c"

static CaDetBox mkbox(float x, float y, float w, float h, int cls, int id, const char *name)
{
    CaDetBox b;
    memset(&b, 0, sizeof(b));
    b.x = x;
    b.y = y;
    b.w = w;
    b.h = h;
    b.cls = cls;
    b.id = id;
    snprintf(b.name, sizeof(b.name), "%s", name);
    return b;
}

static void check_in_unit_range(const CaDetSnapshot *snap)
{
    int i;
    for (i = 0; i < snap->count; ++i) {
        assert(snap->boxes[i].x >= 0.0f && snap->boxes[i].x <= 1.0f);
        assert(snap->boxes[i].y >= 0.0f && snap->boxes[i].y <= 1.0f);
        assert(snap->boxes[i].w >= 0.0f && snap->boxes[i].w <= 1.0f);
        assert(snap->boxes[i].h >= 0.0f && snap->boxes[i].h <= 1.0f);
    }
}

int main(void)
{
    CaDetSnapshot snap;
    CaDetBox boxes[CA_DETECT_OVERLAY_MAX_BOXES];
    CaDetBox b[3];
    int i;

    /* Reading before init yields an empty snapshot, never a crash. */
    ca_detect_overlay_get(&snap);
    assert(snap.seq == 0 && snap.count == 0);

    /* Publishing before init is refused loudly instead of silently dropping. */
    assert(ca_detect_overlay_publish(NULL, 0, 1920, 1080, 1) == CA_ERR);
    assert(g_warned_no_init == 1);

    assert(ca_detect_overlay_init() == CA_OK);
    assert(ca_detect_overlay_init() == CA_OK); /* idempotent */

    ca_detect_overlay_get(&snap);
    assert(snap.seq == 0 && snap.count == 0);

    /* The UI must not draw anything before the first publish. */
    assert(ca_detect_overlay_publish(NULL, 1, 0, 0, 0) == CA_ERR);
    assert(ca_detect_overlay_publish(b, -1, 0, 0, 0) == CA_ERR);
    ca_detect_overlay_get(&snap);
    assert(snap.seq == 0 && snap.count == 0); /* rejected calls do not advance seq */

    /* Two targets, as the HIKFlow algorithm thread would publish them. */
    b[0] = mkbox(0.25f, 0.5f, 0.125f, 0.25f, 0, 1, "person");
    b[1] = mkbox(0.75f, 0.25f, 0.1f, 0.2f, 2, 2, "car");
    b[0].confidence = 0.8234f;
    b[1].confidence = 0.6178f;
    test_now_ms = 2000;
    assert(ca_detect_overlay_publish(b, 2, 1920, 1080, 123456) == CA_OK);
    ca_detect_overlay_get(&snap);
    assert(snap.seq == 1 && snap.count == 2);
    assert(fabsf(snap.boxes[0].confidence - 0.8234f) < 0.00001f);
    assert(fabsf(snap.boxes[1].confidence - 0.6178f) < 0.00001f);
    assert(snap.frame_w == 1920 && snap.frame_h == 1080 && snap.ts_ms == 123456);
    assert(snap.boxes[0].cls == 0 && snap.boxes[0].id == 1);
    assert(strcmp(snap.boxes[0].name, "person") == 0);
    assert(strcmp(snap.boxes[1].name, "car") == 0);
    assert(snap.boxes[1].x > 0.7499f && snap.boxes[1].x < 0.7501f);
    check_in_unit_range(&snap);
    assert(g_last_logged_count == 2 && g_last_log_ms == 2000);

    /* Log throttle: a steady count is rate limited, every change is reported. */
    test_now_ms = 2500;
    assert(ca_detect_overlay_publish(b, 2, 1920, 1080, 1) == CA_OK);
    assert(g_last_log_ms == 2000); /* inside the 1s window, same count: silent */
    test_now_ms = 2600;
    assert(ca_detect_overlay_publish(b, 1, 1920, 1080, 1) == CA_OK);
    assert(g_last_logged_count == 1 && g_last_log_ms == 2600);
    test_now_ms = 3600;
    assert(ca_detect_overlay_publish(b, 1, 1920, 1080, 1) == CA_OK);
    assert(g_last_logged_count == 1 && g_last_log_ms == 3600); /* heartbeat */

    /* An empty publish is how the UI learns the target left the frame. */
    test_now_ms = 3700;
    assert(ca_detect_overlay_publish(NULL, 0, 1920, 1080, 1) == CA_OK);
    ca_detect_overlay_get(&snap);
    assert(snap.count == 0 && snap.seq == 5);
    assert(g_last_logged_count == 0 && g_last_log_ms == 3700);

    /* Hostile model output is clamped so it can never reach the JSON response. */
    {
        volatile float zero = 0.0f;
        memset(&b[0], 0, sizeof(b[0]));
        b[0].x = -1.0f;
        b[0].y = 1.5f;
        b[0].w = 2.0f;
        b[0].h = zero / zero; /* NaN: cJSON would print "nan" and break JSON.parse */
        b[0].cls = 0;
        b[0].id = 9;
        memcpy(b[0].name, "0123456789ABCDEF", CA_DETECT_OVERLAY_NAME_LEN); /* unterminated */
        assert(ca_detect_overlay_publish(b, 1, 1920, 1080, 1) == CA_OK);
        ca_detect_overlay_get(&snap);
        assert(snap.count == 1);
        assert(snap.boxes[0].x == 0.0f && snap.boxes[0].y == 1.0f);
        assert(snap.boxes[0].w == 1.0f && snap.boxes[0].h == 0.0f);
        assert(snap.boxes[0].name[CA_DETECT_OVERLAY_NAME_LEN - 1] == '\0');
        assert(strcmp(snap.boxes[0].name, "0123456789ABCDE") == 0);
        check_in_unit_range(&snap);
    }

    /* Over-long lists are truncated before they are read, so the extra entries
       are never dereferenced and the response size stays bounded. */
    for (i = 0; i < CA_DETECT_OVERLAY_MAX_BOXES; ++i) {
        boxes[i] = mkbox(0.01f * (float)i, 0.02f, 0.03f, 0.04f, i, i + 1, "x");
    }
    assert(ca_detect_overlay_publish(boxes, CA_DETECT_OVERLAY_MAX_BOXES + 5, 800, 600, 7) == CA_OK);
    ca_detect_overlay_get(&snap);
    assert(snap.count == CA_DETECT_OVERLAY_MAX_BOXES);
    assert(snap.boxes[CA_DETECT_OVERLAY_MAX_BOXES - 1].cls == CA_DETECT_OVERLAY_MAX_BOXES - 1);
    check_in_unit_range(&snap);

    /* Non-positive frame sizes are normalised to 0 instead of leaking through. */
    assert(ca_detect_overlay_publish(NULL, 0, -5, 0, 0) == CA_OK);
    ca_detect_overlay_get(&snap);
    assert(snap.seq == 8 && snap.count == 0);
    assert(snap.frame_w == 0 && snap.frame_h == 0);

    puts("PASS: init gating, empty publish, seq monotonic, log throttle, clamping, "
         "NaN rejection, name truncation, list truncation, frame size normalisation");
    return 0;
}
