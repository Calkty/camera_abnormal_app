#ifndef CAMERA_ABNORMAL_RTSP_CLIENT_H
#define CAMERA_ABNORMAL_RTSP_CLIENT_H

#include "config.h"
#include "ring_buffer.h"

typedef struct {
    const AppConfig *cfg;
    PacketRing *ring;
    volatile int *running;
} RtspRecordContext;

void *rtsp_record_thread(void *arg);

#endif
