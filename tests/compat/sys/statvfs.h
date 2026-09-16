#ifndef TEST_STATVFS_H
#define TEST_STATVFS_H
/* Local Windows unit tests only. */
struct statvfs { unsigned long f_bavail, f_frsize; };
static int statvfs(const char *path, struct statvfs *s) { (void)path; s->f_bavail=1000; s->f_frsize=4096; return 0; }
#endif
