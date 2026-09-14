#ifndef CAMERA_ABNORMAL_EVENT_BRIDGE_H
#define CAMERA_ABNORMAL_EVENT_BRIDGE_H

#include <stdint.h>

void camera_abnormal_on_human_alarm(int64_t event_wall_ms, float confidence);

#endif
