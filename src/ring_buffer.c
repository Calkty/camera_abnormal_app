#include "ring_buffer.h"

#include <stdlib.h>

static void packet_free(EncodedPacket *p)
{
    if (p->data) {
        free(p->data);
    }
    memset(p, 0, sizeof(*p));
}

static int packet_copy(EncodedPacket *dst, const EncodedPacket *src)
{
    *dst = *src;
    dst->data = NULL;
    if (src->size > 0) {
        dst->data = (uint8_t *)malloc((size_t)src->size);
        if (!dst->data) {
            return CA_ERR;
        }
        memcpy(dst->data, src->data, (size_t)src->size);
    }
    return CA_OK;
}

static void save_param(uint8_t **dst, int *dst_len, const uint8_t *data, int size)
{
    uint8_t *p = (uint8_t *)malloc((size_t)size);
    if (!p) {
        return;
    }
    memcpy(p, data, (size_t)size);
    free(*dst);
    *dst = p;
    *dst_len = size;
}

static int nal_type(CodecType codec, const uint8_t *annexb, int size)
{
    int off = 0;
    if (size < 5) {
        return -1;
    }
    if (size >= 4 && annexb[0] == 0 && annexb[1] == 0 && annexb[2] == 0 && annexb[3] == 1) {
        off = 4;
    } else if (size >= 3 && annexb[0] == 0 && annexb[1] == 0 && annexb[2] == 1) {
        off = 3;
    }
    if (off <= 0 || off >= size) {
        return -1;
    }
    if (codec == CODEC_H264) {
        return annexb[off] & 0x1f;
    }
    if (codec == CODEC_H265 && off + 1 < size) {
        return (annexb[off] >> 1) & 0x3f;
    }
    return -1;
}

int ring_init(PacketRing *rb, int capacity, int ring_seconds)
{
    memset(rb, 0, sizeof(*rb));
    rb->pkts = (EncodedPacket *)calloc((size_t)capacity, sizeof(EncodedPacket));
    if (!rb->pkts) {
        return CA_ERR;
    }
    rb->capacity = capacity;
    rb->ring_seconds = ring_seconds;
    pthread_mutex_init(&rb->mutex, NULL);
    ca_debug_log(1, "ring_init: capacity=%d ring_seconds=%d", capacity, ring_seconds);
    return CA_OK;
}

void ring_destroy(PacketRing *rb)
{
    int i;
    for (i = 0; i < rb->capacity; i++) {
        packet_free(&rb->pkts[i]);
    }
    free(rb->pkts);
    free(rb->vps);
    free(rb->sps);
    free(rb->pps);
    pthread_mutex_destroy(&rb->mutex);
    memset(rb, 0, sizeof(*rb));
}

int ring_push(PacketRing *rb, const uint8_t *data, int size, int64_t pts_ms,
              int64_t recv_ms, int key_frame, int is_param_set, CodecType codec)
{
    EncodedPacket *slot;
    int idx;
    int type;
    if (!data || size <= 0) {
        return CA_ERR;
    }
    pthread_mutex_lock(&rb->mutex);
    if (rb->count < rb->capacity) {
        idx = (rb->head + rb->count) % rb->capacity;
        rb->count++;
    } else {
        idx = rb->head;
        rb->head = (rb->head + 1) % rb->capacity;
        ca_debug_log(2, "ring overwrite: head=%d capacity=%d recv_ms=%lld",
                     rb->head, rb->capacity, (long long)recv_ms);
        packet_free(&rb->pkts[idx]);
    }
    slot = &rb->pkts[idx];
    slot->data = (uint8_t *)malloc((size_t)size);
    if (!slot->data) {
        pthread_mutex_unlock(&rb->mutex);
        return CA_ERR;
    }
    memcpy(slot->data, data, (size_t)size);
    slot->size = size;
    slot->pts_ms = pts_ms;
    slot->recv_ms = recv_ms;
    slot->key_frame = key_frame;
    slot->is_param_set = is_param_set;
    slot->codec = codec;
    rb->codec = codec;

    type = nal_type(codec, data, size);
    if (codec == CODEC_H264) {
        if (type == 7) {
            save_param(&rb->sps, &rb->sps_len, data, size);
        } else if (type == 8) {
            save_param(&rb->pps, &rb->pps_len, data, size);
        }
    } else if (codec == CODEC_H265) {
        if (type == 32) {
            save_param(&rb->vps, &rb->vps_len, data, size);
        } else if (type == 33) {
            save_param(&rb->sps, &rb->sps_len, data, size);
        } else if (type == 34) {
            save_param(&rb->pps, &rb->pps_len, data, size);
        }
    }
    pthread_mutex_unlock(&rb->mutex);
    return CA_OK;
}

int ring_snapshot(PacketRing *rb, EncodedPacket **out, int *out_count)
{
    EncodedPacket *items;
    int i;
    pthread_mutex_lock(&rb->mutex);
    items = (EncodedPacket *)calloc((size_t)rb->count, sizeof(EncodedPacket));
    if (!items) {
        pthread_mutex_unlock(&rb->mutex);
        return CA_ERR;
    }
    for (i = 0; i < rb->count; i++) {
        int idx = (rb->head + i) % rb->capacity;
        if (packet_copy(&items[i], &rb->pkts[idx]) != CA_OK) {
            packet_array_free(items, i);
            pthread_mutex_unlock(&rb->mutex);
            return CA_ERR;
        }
    }
    *out = items;
    *out_count = rb->count;
    ca_debug_log(2, "ring_snapshot: count=%d capacity=%d codec=%d", rb->count, rb->capacity, rb->codec);
    pthread_mutex_unlock(&rb->mutex);
    return CA_OK;
}

void packet_array_free(EncodedPacket *items, int count)
{
    int i;
    if (!items) {
        return;
    }
    for (i = 0; i < count; i++) {
        free(items[i].data);
    }
    free(items);
}

int ring_get_param_sets(PacketRing *rb, CodecType codec, EncodedPacket **out, int *out_count)
{
    EncodedPacket *items;
    int count = 0;
    pthread_mutex_lock(&rb->mutex);
    count += (codec == CODEC_H265 && rb->vps && rb->vps_len > 0) ? 1 : 0;
    count += (rb->sps && rb->sps_len > 0) ? 1 : 0;
    count += (rb->pps && rb->pps_len > 0) ? 1 : 0;
    items = (EncodedPacket *)calloc((size_t)count, sizeof(EncodedPacket));
    if (!items) {
        pthread_mutex_unlock(&rb->mutex);
        return CA_ERR;
    }
    count = 0;
    if (codec == CODEC_H265 && rb->vps && rb->vps_len > 0) {
        items[count].data = (uint8_t *)malloc((size_t)rb->vps_len);
        if (items[count].data) {
            memcpy(items[count].data, rb->vps, (size_t)rb->vps_len);
            items[count].size = rb->vps_len;
            items[count].codec = codec;
            items[count].is_param_set = 1;
            count++;
        }
    }
    if (rb->sps && rb->sps_len > 0) {
        items[count].data = (uint8_t *)malloc((size_t)rb->sps_len);
        if (items[count].data) {
            memcpy(items[count].data, rb->sps, (size_t)rb->sps_len);
            items[count].size = rb->sps_len;
            items[count].codec = codec;
            items[count].is_param_set = 1;
            count++;
        }
    }
    if (rb->pps && rb->pps_len > 0) {
        items[count].data = (uint8_t *)malloc((size_t)rb->pps_len);
        if (items[count].data) {
            memcpy(items[count].data, rb->pps, (size_t)rb->pps_len);
            items[count].size = rb->pps_len;
            items[count].codec = codec;
            items[count].is_param_set = 1;
            count++;
        }
    }
    *out = items;
    *out_count = count;
    pthread_mutex_unlock(&rb->mutex);
    return CA_OK;
}
