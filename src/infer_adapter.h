#ifndef CAMERA_ABNORMAL_INFER_ADAPTER_H
#define CAMERA_ABNORMAL_INFER_ADAPTER_H

#include "config.h"
#include "event_queue.h"

typedef struct {
    const AppConfig *cfg;
    EventQueue *event_queue;
    volatile int *running;
} InferContext;

int abnormal_event_publish(EventQueue *queue, const char *event_type, float confidence,
                           int64_t event_wall_ms);
void *infer_thread(void *arg);

#endif
