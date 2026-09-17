#ifndef TEST_SYS_TIME_H
#define TEST_SYS_TIME_H
/*
 * Windows-only shim for local deterministic unit tests. Never use in firmware.
 *
 * mingw-w64 already declares struct timeval, struct timespec, clock_gettime,
 * nanosleep and localtime_s through <time.h>, so those names cannot be
 * redeclared here. Instead the calls used by src/ are redirected to the
 * deterministic stubs below with macros, which keeps unit tests independent of
 * the host clock and avoids real sleeping.
 */
#include <time.h>
#include <stdint.h>

extern int64_t test_now_ms;

#ifndef CLOCK_MONOTONIC
#define CLOCK_MONOTONIC 1
#endif

static inline int test_clock_gettime(int id, struct timespec *ts)
{
    (void)id;
    ts->tv_sec = (long)(test_now_ms / 1000);
    ts->tv_nsec = (long)(test_now_ms % 1000) * 1000000;
    return 0;
}
static inline int test_gettimeofday(struct timeval *tv, void *tz)
{
    (void)tz;
    tv->tv_sec = (long)(test_now_ms / 1000);
    tv->tv_usec = (long)(test_now_ms % 1000) * 1000;
    return 0;
}
static inline int test_nanosleep(const struct timespec *req, struct timespec *rem)
{
    (void)rem;
    test_now_ms += req->tv_sec * 1000 + req->tv_nsec / 1000000;
    return 0;
}
static inline struct tm *test_localtime_r(const time_t *t, struct tm *out)
{
    struct tm *p = localtime(t);
    if (!p) {
        return 0;
    }
    *out = *p;
    return out;
}

#define clock_gettime test_clock_gettime
#define gettimeofday test_gettimeofday
#define nanosleep test_nanosleep
#define localtime_r test_localtime_r
#endif

