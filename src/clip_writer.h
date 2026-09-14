#ifndef CAMERA_ABNORMAL_CLIP_WRITER_H
#define CAMERA_ABNORMAL_CLIP_WRITER_H

#include "config.h"
#include "event_queue.h"
#include "ring_buffer.h"
#include "uploader.h"

typedef struct {
    const AppConfig *cfg;
    PacketRing *ring;
    EventQueue *event_queue;
    UploadQueue *upload_queue;
    volatile int *running;
} ClipWriterContext;

void *event_clip_thread(void *arg);

/* Preflight check for the clip output directory; logs the reason on failure. */
int ca_prepare_work_dir(const AppConfig *cfg);

#endif
