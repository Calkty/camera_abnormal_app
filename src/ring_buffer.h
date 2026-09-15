#ifndef CAMERA_ABNORMAL_RING_BUFFER_H
#define CAMERA_ABNORMAL_RING_BUFFER_H

#include "common.h"

#include <pthread.h>

typedef struct {
    uint8_t *data;
    int size;
    int64_t pts_ms;
    int64_t recv_ms;
    int64_t stored_mono_ms; /* eviction uses monotonic time, independent of NTP */
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
    size_t payload_bytes;
    size_t max_bytes;
    size_t peak_payload_bytes;
    unsigned long long evicted_time;
    unsigned long long evicted_bytes;
    unsigned long long evicted_count;
    unsigned long long dropped_oversize;
    unsigned long long alloc_failures;
    int discard;
    CodecType codec;
    uint8_t *vps;
    int vps_len;
    uint8_t *sps;
    int sps_len;
    uint8_t *pps;
    int pps_len;
    pthread_mutex_t mutex;
} PacketRing;

int ring_init(PacketRing *rb, int capacity, int ring_seconds, size_t max_bytes);
void ring_prune(PacketRing *rb);
void ring_destroy(PacketRing *rb);
int ring_push(PacketRing *rb, const uint8_t *data, int size, int64_t pts_ms,
              int64_t recv_ms, int key_frame, int is_param_set, CodecType codec);
int ring_snapshot(PacketRing *rb, EncodedPacket **out, int *out_count);
void packet_array_free(EncodedPacket *items, int count);
int ring_get_param_sets(PacketRing *rb, CodecType codec, EncodedPacket **out, int *out_count);

#endif
