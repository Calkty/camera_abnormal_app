#ifndef CAMERA_ABNORMAL_EVENT_QUEUE_H
#define CAMERA_ABNORMAL_EVENT_QUEUE_H

#include "common.h"

#include <pthread.h>

typedef struct {
    int64_t event_wall_ms;
    int64_t detect_done_ms;
    int channel;
    float confidence;
    char event_type[CA_MAX_EVENT_TYPE];
} AbnormalEvent;

typedef struct {
    AbnormalEvent *items;
    int capacity;
    int head;
    int tail;
    int count;
    int closed;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
} EventQueue;

int event_queue_init(EventQueue *q, int capacity);
void event_queue_destroy(EventQueue *q);
void event_queue_close(EventQueue *q);
int event_queue_push(EventQueue *q, const AbnormalEvent *ev, int block);
int event_queue_pop(EventQueue *q, AbnormalEvent *ev);

#endif
