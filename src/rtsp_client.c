#include "rtsp_client.h"

#include "rtp_h26x.h"

#include <arpa/inet.h>
#include <ctype.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

typedef struct {
    char host[128];
    int port;
    char path[256];
} RtspUrl;

static int parse_rtsp_url(const char *url, RtspUrl *out)
{
    const char *p;
    const char *slash;
    const char *colon;
    int host_len;
    memset(out, 0, sizeof(*out));
    out->port = 554;
    if (strncmp(url, "rtsp://", 7) != 0) {
        return CA_ERR;
    }
    p = url + 7;
    slash = strchr(p, '/');
    if (!slash) {
        return CA_ERR;
    }
    colon = memchr(p, ':', (size_t)(slash - p));
    if (colon) {
        host_len = (int)(colon - p);
        out->port = atoi(colon + 1);
    } else {
        host_len = (int)(slash - p);
    }
    if (host_len <= 0 || host_len >= (int)sizeof(out->host)) {
        return CA_ERR;
    }
    memcpy(out->host, p, (size_t)host_len);
    snprintf(out->path, sizeof(out->path), "%s", slash);
    return CA_OK;
}

static int tcp_connect_host(const char *host, int port)
{
    int fd;
    struct sockaddr_in addr;
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        return -1;
    }
    {
        struct timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    }
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)port);
    if (inet_pton(AF_INET, host, &addr.sin_addr) != 1) {
        close(fd);
        return -1;
    }
    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(fd);
        return -1;
    }
    return fd;
}

static int send_all(int fd, const void *buf, int len)
{
    const uint8_t *p = (const uint8_t *)buf;
    int sent = 0;
    while (sent < len) {
        int n = (int)send(fd, p + sent, (size_t)(len - sent), 0);
        if (n <= 0) {
            return CA_ERR;
        }
        sent += n;
    }
    return CA_OK;
}

static int recv_until_header(int fd, char *buf, int cap)
{
    int len = 0;
    char *cl;
    int body_len = 0;
    while (len < cap - 1) {
        int n = (int)recv(fd, buf + len, 1, 0);
        if (n <= 0) {
            return CA_ERR;
        }
        len += n;
        buf[len] = '\0';
        if (len >= 4 && strstr(buf, "\r\n\r\n")) {
            cl = strstr(buf, "Content-Length:");
            if (!cl) {
                cl = strstr(buf, "Content-length:");
            }
            if (cl) {
                cl += 15;
                while (*cl == ' ') {
                    cl++;
                }
                body_len = atoi(cl);
            }
            while (body_len > 0 && len < cap - 1) {
                n = (int)recv(fd, buf + len, (size_t)((body_len < cap - 1 - len) ? body_len : cap - 1 - len), 0);
                if (n <= 0) {
                    return CA_ERR;
                }
                len += n;
                body_len -= n;
                buf[len] = '\0';
            }
            return len;
        }
    }
    return CA_ERR;
}

static int rtsp_request(int fd, int *cseq, const char *method, const char *url,
                        const char *extra, char *resp, int resp_cap)
{
    char req[1024];
    int len;
    len = snprintf(req, sizeof(req), "%s %s RTSP/1.0\r\nCSeq: %d\r\n%s\r\n",
                   method, url, (*cseq)++, extra ? extra : "");
    if (len <= 0 || len >= (int)sizeof(req)) {
        return CA_ERR;
    }
    if (send_all(fd, req, len) != CA_OK) {
        return CA_ERR;
    }
    if (recv_until_header(fd, resp, resp_cap) < 0) {
        return CA_ERR;
    }
    if (!strstr(resp, "RTSP/1.0 200")) {
        ca_log("ERR", "RTSP %s failed: %.160s", method, resp);
        return CA_ERR;
    }
    return CA_OK;
}

static CodecType codec_from_sdp(const char *sdp, CodecType configured)
{
    if (configured != CODEC_UNKNOWN) {
        return configured;
    }
    if (strstr(sdp, "H265") || strstr(sdp, "H.265") || strstr(sdp, "HEVC")) {
        return CODEC_H265;
    }
    if (strstr(sdp, "H264") || strstr(sdp, "H.264")) {
        return CODEC_H264;
    }
    return CODEC_H264;
}

static void setup_url_from_sdp(const char *sdp, const char *base_url, char *out, int out_cap)
{
    const char *video = strstr(sdp, "m=video");
    const char *p = video ? strstr(video, "a=control:") : strstr(sdp, "a=control:");
    const char *next_media;
    char control[256];
    int len = 0;
    if (!p) {
        snprintf(out, (size_t)out_cap, "%s", base_url);
        return;
    }
    if (video) {
        next_media = strstr(video + 1, "\nm=");
        if (next_media && p > next_media) {
            p = strstr(sdp, "a=control:");
            if (!p) {
                snprintf(out, (size_t)out_cap, "%s", base_url);
                return;
            }
        }
    }
    p += 10;
    while (p[len] && p[len] != '\r' && p[len] != '\n' && len < (int)sizeof(control) - 1) {
        len++;
    }
    memcpy(control, p, (size_t)len);
    control[len] = '\0';
    if (strncmp(control, "rtsp://", 7) == 0) {
        snprintf(out, (size_t)out_cap, "%s", control);
    } else if (strcmp(control, "*") == 0) {
        snprintf(out, (size_t)out_cap, "%s", base_url);
    } else if (base_url[strlen(base_url) - 1] == '/') {
        snprintf(out, (size_t)out_cap, "%s%s", base_url, control);
    } else {
        snprintf(out, (size_t)out_cap, "%s/%s", base_url, control);
    }
}

static int parse_session(const char *resp, char *session, int cap)
{
    const char *p = strstr(resp, "Session:");
    const char *e;
    int len;
    if (!p) {
        return CA_ERR;
    }
    p += 8;
    while (*p == ' ') {
        p++;
    }
    e = p;
    while (*e && *e != ';' && *e != '\r' && *e != '\n') {
        e++;
    }
    len = (int)(e - p);
    if (len <= 0 || len >= cap) {
        return CA_ERR;
    }
    memcpy(session, p, (size_t)len);
    session[len] = '\0';
    return CA_OK;
}

static int read_exact(int fd, uint8_t *buf, int len)
{
    int got = 0;
    while (got < len) {
        int n = (int)recv(fd, buf + got, (size_t)(len - got), 0);
        if (n <= 0) {
            return CA_ERR;
        }
        got += n;
    }
    return CA_OK;
}

static void rtsp_loop(const AppConfig *cfg, PacketRing *ring, volatile int *running)
{
    RtspUrl u;
    int fd = -1;
    int cseq = 1;
    char full_url[CA_MAX_URL];
    char setup_url[CA_MAX_URL * 2];
    char resp[4096];
    char session[128];
    char extra[256];
    CodecType codec;
    RtpH26xParser parser;
    int64_t last_stat_ms = 0;
    int rtp_count = 0;
    int rtp_bytes = 0;

    if (parse_rtsp_url(cfg->rtsp_url, &u) != CA_OK) {
        ca_log("ERR", "bad rtsp_url: %s", cfg->rtsp_url);
        ca_sleep_ms(3000);
        return;
    }
    /* HEOP exposes the device RTSP service through BR0_IP, not loopback. */
    if (strcmp(u.host, "127.0.0.1") == 0) {
        const char *br0_ip = getenv("BR0_IP");
        struct in_addr br0_addr;
        if (br0_ip && inet_pton(AF_INET, br0_ip, &br0_addr) == 1) {
            snprintf(u.host, sizeof(u.host), "%s", br0_ip);
            ca_log("INFO", "RTSP loopback resolved through BR0_IP=%s", u.host);
        } else {
            ca_log("WARN", "BR0_IP is unavailable; RTSP keeps loopback address");
        }
    }
    snprintf(full_url, sizeof(full_url), "rtsp://%s:%d%s", u.host, u.port, u.path);
    fd = tcp_connect_host(u.host, u.port);
    if (fd < 0) {
        ca_log("ERR", "connect RTSP %s:%d failed", u.host, u.port);
        ca_sleep_ms(3000);
        return;
    }
    ca_log("INFO", "RTSP connected: %s", full_url);
    if (rtsp_request(fd, &cseq, "DESCRIBE", full_url,
                     "Accept: application/sdp\r\n", resp, sizeof(resp)) != CA_OK) {
        close(fd);
        return;
    }
    codec = codec_from_sdp(resp, cfg->codec);
    setup_url_from_sdp(resp, full_url, setup_url, sizeof(setup_url));
    ca_debug_log(1, "RTSP DESCRIBE ok: codec=%s setup_url=%s",
                 codec == CODEC_H265 ? "H265" : "H264", setup_url);
    snprintf(extra, sizeof(extra), "Transport: RTP/AVP/TCP;unicast;interleaved=0-1\r\n");
    if (rtsp_request(fd, &cseq, "SETUP", setup_url, extra, resp, sizeof(resp)) != CA_OK ||
        parse_session(resp, session, sizeof(session)) != CA_OK) {
        close(fd);
        return;
    }
    snprintf(extra, sizeof(extra), "Session: %s\r\nRange: npt=0.000-\r\n", session);
    if (rtsp_request(fd, &cseq, "PLAY", full_url, extra, resp, sizeof(resp)) != CA_OK) {
        close(fd);
        return;
    }
    ca_log("INFO", "RTSP PLAY ok, codec=%s", codec == CODEC_H265 ? "H265" : "H264");
    rtp_h26x_init(&parser, codec);
    last_stat_ms = ca_now_ms();
    while (*running) {
        uint8_t hdr[4];
        uint8_t *payload;
        int len;
        if (read_exact(fd, hdr, 4) != CA_OK) {
            ca_log("WARN", "DIAG RTSP header read ended: errno=%d (may be EOF)", errno);
            break;
        }
        if (hdr[0] != '$') {
            continue;
        }
        len = ((int)hdr[2] << 8) | hdr[3];
        if (len <= 0 || len > 262144) {
            break;
        }
        payload = (uint8_t *)malloc((size_t)len);
        if (!payload) {
            ca_log("ERR", "DIAG RTSP malloc failed: bytes=%d errno=%d", len, errno);
            break;
        }
        if (read_exact(fd, payload, len) != CA_OK) {
            ca_log("WARN", "DIAG RTSP payload read ended: bytes=%d errno=%d (may be EOF)", len, errno);
            free(payload);
            break;
        }
        if (hdr[1] == 0) {
            rtp_h26x_push_rtp(&parser, ring, payload, len, ca_now_ms());
            rtp_count++;
            rtp_bytes += len;
            if (ca_now_ms() - last_stat_ms >= 5000) {
                ca_log("INFO", "DIAG RTSP: elapsed_ms=%lld rtp_packets=%d rtp_bytes=%d fu_len=%d fu_cap=%d",
                       (long long)(ca_now_ms() - last_stat_ms), rtp_count, rtp_bytes,
                       parser.fu_len, parser.fu_cap);
                rtp_count = 0;
                rtp_bytes = 0;
                last_stat_ms = ca_now_ms();
            }
        }
        free(payload);
    }
    rtp_h26x_destroy(&parser);
    close(fd);
    ca_log("WARN", "RTSP loop ended, reconnecting if still running");
}

void *rtsp_record_thread(void *arg)
{
    RtspRecordContext *ctx = (RtspRecordContext *)arg;
    while (*ctx->running) {
        rtsp_loop(ctx->cfg, ctx->ring, ctx->running);
    }
    return NULL;
}
