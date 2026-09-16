/* Deterministic syscall simulation: no device files are accessed. */
#include "../src/config.h"
#include <assert.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
int64_t test_now_ms=0;
int g_ca_debug_level=0;
#ifndef O_DIRECTORY
#define O_DIRECTORY 0x100000
#define O_NOFOLLOW 0x200000
#define O_CLOEXEC 0x400000
#define O_NONBLOCK 0x800000
#define AT_SYMLINK_NOFOLLOW 0x100
#endif
#ifndef S_ISREG
#define S_ISREG(m) (((m)&S_IFMT)==S_IFREG)
#endif
static const char *names[]={"old.h264","old.json","keep.txt","subdir.h265","link.json"};
static int alive[5], pos, marked, bad_marker, fail_delete, fail_marker;
static struct dirent de;
static int sim_open(const char *p,int f,...) { (void)p;assert(f&O_NOFOLLOW);return 10; }
static int sim_openat(int fd,const char *p,int f,...) {
    assert(fd==10 && (f&O_NOFOLLOW));
    if(!strcmp(p,"events")) return 20;
    assert(!strcmp(p,".events_clear_once_v1.done"));
    if(f&O_CREAT) { if(fail_marker){errno=ENOSPC;return -1;} assert(!marked);marked=1;return 30; }
    if(marked)return 30;errno=ENOENT;return -1;
}
static DIR *sim_fdopendir(int fd) { assert(fd==20);pos=0;return (DIR*)1; }
static struct dirent *sim_readdir(DIR *d) { (void)d;while(pos<5&&!alive[pos])pos++;if(pos==5)return NULL;de.d_name=(char *)names[pos++];return &de; }
static int sim_statat(int fd,const char *p,struct stat *s,int flags) {
    int i;assert(fd==20 && flags==AT_SYMLINK_NOFOLLOW);
    for(i=0;i<5;i++)if(!strcmp(p,names[i])){s->st_mode=(i<3?S_IFREG:(i==3?S_IFDIR:0120000));return 0;}
    assert(0);return -1;
}
static int sim_unlinkat(int fd,const char *p,int flags) {
    int i;(void)flags;if(fd==10){marked=0;return 0;}assert(fd==20);
    if(fail_delete){errno=EACCES;return -1;}
    for(i=0;i<5;i++)if(!strcmp(p,names[i])){assert(i<2);alive[i]=0;return 0;}assert(0);return -1;
}
static int sim_fstat(int fd,struct stat *s){assert(fd==30);s->st_mode=S_IFREG;s->st_size=bad_marker?0:5;return 0;}
static int sim_read(int fd,void *p,unsigned n){assert(fd==30&&n>=5);memcpy(p,"done\n",5);return 5;}
static int sim_write(int fd,const void *p,unsigned n){assert(fd==30&&n==5&&!memcmp(p,"done\n",5));return 5;}
static int sim_sync(int fd){(void)fd;return 0;}
static int sim_close(int fd){(void)fd;return 0;}
static int sim_closedir(DIR *d){(void)d;return 0;}
#define open sim_open
#define openat sim_openat
#define fdopendir sim_fdopendir
#define readdir sim_readdir
#define fstatat sim_statat
#define unlinkat sim_unlinkat
#define fstat sim_fstat
#define read sim_read
#define write sim_write
#define fsync sim_sync
#define close sim_close
#define closedir sim_closedir
#include "../src/startup_cleanup.c"
static void reset(void){int i;for(i=0;i<5;i++)alive[i]=1;marked=bad_marker=fail_delete=fail_marker=0;}
int main(void){
    AppConfig cfg;memset(&cfg,0,sizeof(cfg));strcpy(cfg.work_dir,CLEAR_WORK_DIR);
    reset();assert(clear_events_once(&cfg)==CA_OK&&alive[0]);
    cfg.clear_events_once=1;assert(clear_events_once(&cfg)==CA_OK);
    assert(marked&&!alive[0]&&!alive[1]&&alive[2]&&alive[3]&&alive[4]);
    alive[0]=alive[1]=1;assert(clear_events_once(&cfg)==CA_OK&&alive[0]&&alive[1]);
    reset();fail_delete=1;assert(clear_events_once(&cfg)==CA_ERR&&!marked&&alive[0]);
    fail_delete=0;assert(clear_events_once(&cfg)==CA_OK&&marked);
    reset();fail_marker=1;assert(clear_events_once(&cfg)==CA_ERR&&!marked&&!alive[0]);
    fail_marker=0;assert(clear_events_once(&cfg)==CA_OK&&marked);
    reset();marked=bad_marker=1;assert(clear_events_once(&cfg)==CA_ERR&&alive[0]);
    reset();strcpy(cfg.work_dir,"/tmp/other");assert(clear_events_once(&cfg)==CA_ERR&&alive[0]);
    puts("PASS: one-shot cleanup, restart preservation, extension/directory/symlink filtering, delete/marker failure, invalid marker, exact path guard");return 0;
}
