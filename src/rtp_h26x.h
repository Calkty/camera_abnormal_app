#ifndef CAMERA_ABNORMAL_RTP_H26X_H
#define CAMERA_ABNORMAL_RTP_H26X_H

#include "common.h"
#include "ring_buffer.h"

typedef struct {
    CodecType codec;
    uint32_t first_rtp_ts;
    int have_first_ts;
    uint8_t *fu_buf;
    int fu_len;
    int fu_cap;
} RtpH26xParser;

void rtp_h26x_init(RtpH26xParser *p, CodecType codec);
void rtp_h26x_destroy(RtpH26xParser *p);
int rtp_h26x_push_rtp(RtpH26xParser *p, PacketRing *ring,
                      const uint8_t *rtp, int rtp_len, int64_t recv_ms);

#endif
