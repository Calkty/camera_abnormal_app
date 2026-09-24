#ifndef CAMERA_ABNORMAL_DETECT_OVERLAY_H
#define CAMERA_ABNORMAL_DETECT_OVERLAY_H

#include "common.h"

#include <pthread.h>

/*
 * Latest inference result, published by the HIKFlow algorithm thread and read
 * by the ISAPI thread that serves
 * /ISAPI/Custom/OpenPlatform/extern/cameraAbnormal/detections.
 *
 * Boxes use the same normalized [0,1] frame space as
 * OPDEVSDK_POS_TARGET_ST.region, so the browser scales them by its video
 * canvas size and never needs to know the capture resolution.
 */

#define CA_DETECT_OVERLAY_MAX_BOXES 16
#define CA_DETECT_OVERLAY_NAME_LEN 16

typedef struct {
    float x; /*!< normalized left edge, range [0,1] */
    float y; /*!< normalized top edge, range [0,1] */
    float w; /*!< normalized width, range [0,1] */
    float h; /*!< normalized height, range [0,1] */
    int cls; /*!< model class index */
    float confidence; /*!< raw score of this target, not an event-wide score */
    int id;  /*!< target id reported by the demo */
    char name[CA_DETECT_OVERLAY_NAME_LEN]; /*!< class name, always NUL terminated */
} CaDetBox;

typedef struct {
    unsigned long long seq; /*!< 0 until the first publish, then monotonic */
    int64_t ts_ms;          /*!< frame timestamp reported by the demo */
    int frame_w;            /*!< capture frame size, diagnostics only */
    int frame_h;
    int count;
    CaDetBox boxes[CA_DETECT_OVERLAY_MAX_BOXES];
} CaDetSnapshot;

/*!< Create the snapshot lock. Idempotent; call before the inference threads start. */
int ca_detect_overlay_init(void);

/**
 * @brief  Replace the snapshot with the newest result.
 *
 * Publishing an empty list (count == 0, boxes may be NULL) is valid and is how
 * the UI learns that the target disappeared. Boxes past
 * CA_DETECT_OVERLAY_MAX_BOXES are dropped. Coordinates are clamped into [0,1]
 * and class names are truncated, so a bad model output cannot reach the browser.
 */
int ca_detect_overlay_publish(const CaDetBox *boxes, int count, int frame_w,
                              int frame_h, int64_t ts_ms);

/*!< Copy the current snapshot. Yields seq == 0, count == 0 before the first publish. */
void ca_detect_overlay_get(CaDetSnapshot *out);

#endif
