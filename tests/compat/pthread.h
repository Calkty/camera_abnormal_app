#ifndef TEST_PTHREAD_H
#define TEST_PTHREAD_H
/* Single-thread test lock: checks balanced ownership; no concurrency simulation. */
#include <assert.h>
typedef int pthread_mutex_t;
typedef unsigned long pthread_t;
typedef int pthread_cond_t;
int pthread_create(pthread_t *, const void *, void *(*)(void *), void *);
int pthread_join(pthread_t, void **);
static int pthread_mutex_trylock(pthread_mutex_t *m) { if (*m) return 1; *m=1; return 0; }
static int pthread_mutex_init(pthread_mutex_t *m, void *a) { (void)a; *m=0; return 0; }
static int pthread_mutex_lock(pthread_mutex_t *m) { assert(!*m); *m=1; return 0; }
static int pthread_mutex_unlock(pthread_mutex_t *m) { assert(*m); *m=0; return 0; }
static int pthread_mutex_destroy(pthread_mutex_t *m) { assert(!*m); return 0; }
#endif
