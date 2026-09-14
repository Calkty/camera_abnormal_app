#ifndef CAMERA_ABNORMAL_UPLOADER_H
#define CAMERA_ABNORMAL_UPLOADER_H

#include "config.h"

#include <pthread.h>

typedef struct {
    char video_path[CA_MAX_FULL_PATH];
    char meta_path[CA_MAX_FULL_PATH];
    char camera_id[CA_MAX_ID];
    char event_type[CA_MAX_EVENT_TYPE];
    int64_t event_wall_ms;
    CodecType codec;
} UploadJob;

typedef struct {
    UploadJob *items;
    int capacity;
    int head;
    int tail;
    int count;
    int closed;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
} UploadQueue;

typedef struct {
    const AppConfig *cfg;
    UploadQueue *queue;
    volatile int *running;
} UploadContext;

int upload_queue_init(UploadQueue *q, int capacity);
void upload_queue_destroy(UploadQueue *q);
void upload_queue_close(UploadQueue *q);
int upload_queue_push(UploadQueue *q, const UploadJob *job, int block);
int upload_queue_pop(UploadQueue *q, UploadJob *job);
void *upload_thread(void *arg);

#endif
