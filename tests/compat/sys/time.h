#ifndef TEST_SYS_TIME_H
#define TEST_SYS_TIME_H
/* Windows-only shim for local deterministic unit tests. Never use in firmware. */
#include <time.h>
#include <stdint.h>
struct timeval { long tv_sec; long tv_usec; };
struct timespec { long tv_sec; long tv_nsec; };
#define CLOCK_MONOTONIC 1
extern int64_t test_now_ms;
static int clock_gettime(int id, struct timespec *ts) {
    (void)id; ts->tv_sec = (long)(test_now_ms / 1000);
    ts->tv_nsec = (long)(test_now_ms % 1000) * 1000000; return 0;
}
static int gettimeofday(struct timeval *tv, void *tz) {
    (void)tz; tv->tv_sec = (long)(test_now_ms / 1000);
    tv->tv_usec = (long)(test_now_ms % 1000) * 1000; return 0;
}
static int nanosleep(const struct timespec *req, struct timespec *rem) {
    (void)rem; test_now_ms += req->tv_sec * 1000 + req->tv_nsec / 1000000; return 0;
}
static struct tm *localtime_r(const time_t *t, struct tm *out) {
    struct tm *p = localtime(t); if (!p) return 0; *out = *p; return out;
}
#endif
