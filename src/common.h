#ifndef CAMERA_ABNORMAL_COMMON_H
#define CAMERA_ABNORMAL_COMMON_H

#include <errno.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#define CA_OK 0
#define CA_ERR (-1)
#define CA_TRUE 1
#define CA_FALSE 0

#define CA_MAX_PATH 256
#define CA_MAX_FULL_PATH 1024
#define CA_MAX_URL 512
#define CA_MAX_ID 64
#define CA_MAX_EVENT_TYPE 64

extern int g_ca_debug_level;

typedef enum {
    CODEC_UNKNOWN = 0,
    CODEC_H264 = 1,
    CODEC_H265 = 2
} CodecType;

static inline int64_t ca_now_ms(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (int64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

static inline void ca_sleep_ms(int ms)
{
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (long)(ms % 1000) * 1000000L;
    while (nanosleep(&ts, &ts) < 0 && errno == EINTR) {
    }
}

static inline void ca_log(const char *level, const char *fmt, ...)
{
    va_list ap;
    time_t now = time(NULL);
    struct tm tmv;
    localtime_r(&now, &tmv);
    fprintf(stderr, "%04d-%02d-%02d %02d:%02d:%02d [%s] ",
            tmv.tm_year + 1900, tmv.tm_mon + 1, tmv.tm_mday,
            tmv.tm_hour, tmv.tm_min, tmv.tm_sec, level);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, "\n");
}

static inline void ca_set_debug_level(int level)
{
    g_ca_debug_level = level;
}

static inline int ca_debug_enabled(int level)
{
    return g_ca_debug_level >= level;
}

static inline void ca_debug_log(int level, const char *fmt, ...)
{
    va_list ap;
    time_t now;
    struct tm tmv;
    if (!ca_debug_enabled(level)) {
        return;
    }
    now = time(NULL);
    localtime_r(&now, &tmv);
    fprintf(stderr, "%04d-%02d-%02d %02d:%02d:%02d [DEBUG%d] ",
            tmv.tm_year + 1900, tmv.tm_mon + 1, tmv.tm_mday,
            tmv.tm_hour, tmv.tm_min, tmv.tm_sec, level);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, "\n");
}

#endif
