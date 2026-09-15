#include "rtp_h26x.h"

#include <arpa/inet.h>
#include <stdlib.h>

static const uint8_t g_start_code[] = {0x00, 0x00, 0x00, 0x01};

static int ensure_fu(RtpH26xParser *p, int need)
{
    uint8_t *n;
    int cap = p->fu_cap;
    if (cap >= need) {
        return CA_OK;
    }
    if (cap <= 0) {
        cap = 4096;
    }
    while (cap < need) {
        cap *= 2;
    }
    n = (uint8_t *)realloc(p->fu_buf, (size_t)cap);
    if (!n) {
        return CA_ERR;
    }
    p->fu_buf = n;
    p->fu_cap = cap;
    return CA_OK;
}

static int push_annexb(PacketRing *ring, CodecType codec, const uint8_t *nal, int nal_len,
                       int64_t pts_ms, int64_t recv_ms)
{
    uint8_t *buf;
    int nal_type;
    int key = 0;
    int param = 0;
    int rc;
    if (!nal || nal_len <= 0) {
        return CA_ERR;
    }
    if (codec == CODEC_H264) {
        nal_type = nal[0] & 0x1f;
        key = (nal_type == 5);
        param = (nal_type == 7 || nal_type == 8);
    } else {
        nal_type = (nal[0] >> 1) & 0x3f;
        key = (nal_type >= 16 && nal_type <= 21);
        param = (nal_type == 32 || nal_type == 33 || nal_type == 34);
    }
    buf = (uint8_t *)malloc((size_t)nal_len + sizeof(g_start_code));
    if (!buf) {
        return CA_ERR;
    }
    memcpy(buf, g_start_code, sizeof(g_start_code));
    memcpy(buf + sizeof(g_start_code), nal, (size_t)nal_len);
    key = key || param;
    rc = ring_push(ring, buf, nal_len + (int)sizeof(g_start_code), pts_ms, recv_ms, key, param, codec);
    free(buf);
    return rc;
}

static int parse_rtp_header(const uint8_t *rtp, int rtp_len, const uint8_t **payload,
                            int *payload_len, uint32_t *ts)
{
    int cc;
    int off;
    if (rtp_len < 12 || ((rtp[0] >> 6) & 0x03) != 2) {
        return CA_ERR;
    }
    cc = rtp[0] & 0x0f;
    off = 12 + cc * 4;
    if (rtp_len < off) {
        return CA_ERR;
    }
    *ts = ((uint32_t)rtp[4] << 24) | ((uint32_t)rtp[5] << 16) |
          ((uint32_t)rtp[6] << 8) | (uint32_t)rtp[7];
    if (rtp[0] & 0x10) {
        uint16_t ext_len_words;
        if (rtp_len < off + 4) {
            return CA_ERR;
        }
        ext_len_words = ((uint16_t)rtp[off + 2] << 8) | rtp[off + 3];
        off += 4 + ext_len_words * 4;
        if (rtp_len < off) {
            return CA_ERR;
        }
    }
    *payload = rtp + off;
    *payload_len = rtp_len - off;
    return CA_OK;
}

static int push_h264(RtpH26xParser *p, PacketRing *ring, const uint8_t *pl, int len,
                     int64_t pts_ms, int64_t recv_ms)
{
    int type;
    if (len <= 0) {
        return CA_ERR;
    }
    type = pl[0] & 0x1f;
    if (type >= 1 && type <= 23) {
        return push_annexb(ring, CODEC_H264, pl, len, pts_ms, recv_ms);
    }
    if (type == 24) {
        int off = 1;
        while (off + 2 <= len) {
            int nlen = ((int)pl[off] << 8) | pl[off + 1];
            off += 2;
            if (nlen <= 0 || off + nlen > len) {
                return CA_ERR;
            }
            push_annexb(ring, CODEC_H264, pl + off, nlen, pts_ms, recv_ms);
            off += nlen;
        }
        return CA_OK;
    }
    if (type == 28 && len >= 2) {
        int start = pl[1] & 0x80;
        int end = pl[1] & 0x40;
        uint8_t reconstructed = (pl[0] & 0xe0) | (pl[1] & 0x1f);
        if (start) {
            p->fu_len = 0;
            if (ensure_fu(p, 1 + len - 2) != CA_OK) {
                return CA_ERR;
            }
            p->fu_buf[p->fu_len++] = reconstructed;
        } else if (p->fu_len <= 0) {
            return CA_ERR;
        }
        if (ensure_fu(p, p->fu_len + len - 2) != CA_OK) {
            return CA_ERR;
        }
        memcpy(p->fu_buf + p->fu_len, pl + 2, (size_t)(len - 2));
        p->fu_len += len - 2;
        if (end) {
            int rc = push_annexb(ring, CODEC_H264, p->fu_buf, p->fu_len, pts_ms, recv_ms);
            p->fu_len = 0;
            return rc;
        }
        return CA_OK;
    }
    return CA_OK;
}

static int push_h265(RtpH26xParser *p, PacketRing *ring, const uint8_t *pl, int len,
                     int64_t pts_ms, int64_t recv_ms)
{
    int type;
    if (len < 2) {
        return CA_ERR;
    }
    type = (pl[0] >> 1) & 0x3f;
    if (type <= 47) {
        return push_annexb(ring, CODEC_H265, pl, len, pts_ms, recv_ms);
    }
    if (type == 48) {
        int off = 2;
        while (off + 2 <= len) {
            int nlen = ((int)pl[off] << 8) | pl[off + 1];
            off += 2;
            if (nlen <= 0 || off + nlen > len) {
                return CA_ERR;
            }
            push_annexb(ring, CODEC_H265, pl + off, nlen, pts_ms, recv_ms);
            off += nlen;
        }
        return CA_OK;
    }
    if (type == 49 && len >= 3) {
        int start = pl[2] & 0x80;
        int end = pl[2] & 0x40;
        uint8_t nal0 = (uint8_t)((pl[0] & 0x81) | ((pl[2] & 0x3f) << 1));
        uint8_t nal1 = pl[1];
        if (start) {
            p->fu_len = 0;
            if (ensure_fu(p, 2 + len - 3) != CA_OK) {
                return CA_ERR;
            }
            p->fu_buf[p->fu_len++] = nal0;
            p->fu_buf[p->fu_len++] = nal1;
        } else if (p->fu_len <= 0) {
            return CA_ERR;
        }
        if (ensure_fu(p, p->fu_len + len - 3) != CA_OK) {
            return CA_ERR;
        }
        memcpy(p->fu_buf + p->fu_len, pl + 3, (size_t)(len - 3));
        p->fu_len += len - 3;
        if (end) {
            int rc = push_annexb(ring, CODEC_H265, p->fu_buf, p->fu_len, pts_ms, recv_ms);
            p->fu_len = 0;
            return rc;
        }
        return CA_OK;
    }
    return CA_OK;
}

void rtp_h26x_init(RtpH26xParser *p, CodecType codec)
{
    memset(p, 0, sizeof(*p));
    p->codec = codec;
}

void rtp_h26x_destroy(RtpH26xParser *p)
{
    free(p->fu_buf);
    memset(p, 0, sizeof(*p));
}

int rtp_h26x_push_rtp(RtpH26xParser *p, PacketRing *ring,
                      const uint8_t *rtp, int rtp_len, int64_t recv_ms)
{
    const uint8_t *pl;
    int pl_len;
    uint32_t ts;
    int64_t pts_ms;
    if (parse_rtp_header(rtp, rtp_len, &pl, &pl_len, &ts) != CA_OK) {
        return CA_ERR;
    }
    if (!p->have_first_ts) {
        p->first_rtp_ts = ts;
        p->have_first_ts = 1;
    }
    pts_ms = (int64_t)((uint32_t)(ts - p->first_rtp_ts)) / 90;
    if (p->codec == CODEC_H264) {
        return push_h264(p, ring, pl, pl_len, pts_ms, recv_ms);
    }
    if (p->codec == CODEC_H265) {
        return push_h265(p, ring, pl, pl_len, pts_ms, recv_ms);
    }
    return CA_ERR;
}
