#ifndef CAMERA_ABNORMAL_RING_BUFFER_H
#define CAMERA_ABNORMAL_RING_BUFFER_H

#include "common.h"

#include <pthread.h>

typedef struct {
    uint8_t *data;
    int size;
    int64_t pts_ms;
    int64_t recv_ms;
    int key_frame;
    int is_param_set;
    CodecType codec;
} EncodedPacket;

typedef struct {
    EncodedPacket *pkts;
    int capacity;
    int head;
    int count;
    int ring_seconds;
    CodecType codec;
    uint8_t *vps;
    int vps_len;
    uint8_t *sps;
    int sps_len;
    uint8_t *pps;
    int pps_len;
    pthread_mutex_t mutex;
} PacketRing;

int ring_init(PacketRing *rb, int capacity, int ring_seconds);
void ring_destroy(PacketRing *rb);
int ring_push(PacketRing *rb, const uint8_t *data, int size, int64_t pts_ms,
              int64_t recv_ms, int key_frame, int is_param_set, CodecType codec);
int ring_snapshot(PacketRing *rb, EncodedPacket **out, int *out_count);
void packet_array_free(EncodedPacket *items, int count);
int ring_get_param_sets(PacketRing *rb, CodecType codec, EncodedPacket **out, int *out_count);

#endif
