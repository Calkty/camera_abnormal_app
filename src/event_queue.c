#include "event_queue.h"

#include <stdlib.h>

int event_queue_init(EventQueue *q, int capacity)
{
    memset(q, 0, sizeof(*q));
    q->items = (AbnormalEvent *)calloc((size_t)capacity, sizeof(AbnormalEvent));
    if (!q->items) {
        return CA_ERR;
    }
    q->capacity = capacity;
    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->not_empty, NULL);
    pthread_cond_init(&q->not_full, NULL);
    return CA_OK;
}

void event_queue_destroy(EventQueue *q)
{
    if (q->items) {
        free(q->items);
    }
    pthread_mutex_destroy(&q->mutex);
    pthread_cond_destroy(&q->not_empty);
    pthread_cond_destroy(&q->not_full);
    memset(q, 0, sizeof(*q));
}

void event_queue_close(EventQueue *q)
{
    pthread_mutex_lock(&q->mutex);
    q->closed = 1;
    pthread_cond_broadcast(&q->not_empty);
    pthread_cond_broadcast(&q->not_full);
    pthread_mutex_unlock(&q->mutex);
}

int event_queue_push(EventQueue *q, const AbnormalEvent *ev, int block)
{
    pthread_mutex_lock(&q->mutex);
    while (!q->closed && q->count == q->capacity && block) {
        pthread_cond_wait(&q->not_full, &q->mutex);
    }
    if (q->closed || q->count == q->capacity) {
        int closed = q->closed;
        int count = q->count;
        int capacity = q->capacity;
        ca_debug_log(1, "event_queue_push rejected: closed=%d count=%d capacity=%d block=%d",
                     closed, count, capacity, block);
        pthread_mutex_unlock(&q->mutex);
        return CA_ERR;
    }
    q->items[q->tail] = *ev;
    q->tail = (q->tail + 1) % q->capacity;
    q->count++;
    ca_debug_log(2, "event_queue_push: type=%s event_wall_ms=%lld count=%d/%d",
                 ev->event_type, (long long)ev->event_wall_ms, q->count, q->capacity);
    pthread_cond_signal(&q->not_empty);
    pthread_mutex_unlock(&q->mutex);
    return CA_OK;
}

int event_queue_pop(EventQueue *q, AbnormalEvent *ev)
{
    pthread_mutex_lock(&q->mutex);
    while (!q->closed && q->count == 0) {
        pthread_cond_wait(&q->not_empty, &q->mutex);
    }
    if (q->count == 0) {
        pthread_mutex_unlock(&q->mutex);
        return CA_ERR;
    }
    *ev = q->items[q->head];
    q->head = (q->head + 1) % q->capacity;
    q->count--;
    ca_debug_log(2, "event_queue_pop: type=%s event_wall_ms=%lld count=%d/%d",
                 ev->event_type, (long long)ev->event_wall_ms, q->count, q->capacity);
    pthread_cond_signal(&q->not_full);
    pthread_mutex_unlock(&q->mutex);
    return CA_OK;
}
