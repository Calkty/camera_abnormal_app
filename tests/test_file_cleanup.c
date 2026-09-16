#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/stat.h>
#include <direct.h>
#include "../src/common.h"
#include "../src/event_files.h"
#include "../src/clip_writer.h"
int64_t test_now_ms=1000;
int g_ca_debug_level=0;
static int fail_write, fail_flush, fail_close;
static size_t injected_fwrite(const void *p,size_t s,size_t n,FILE *f) {
    if (fail_write) { fail_write=0; errno=ENOSPC; return 0; } return fwrite(p,s,n,f);
}
static int injected_fflush(FILE *f) {
    if (fail_flush) { fail_flush=0; errno=ENOSPC; return EOF; } return fflush(f);
}
static int injected_fclose(FILE *f) {
    int rc=fclose(f);
    if (fail_close && --fail_close==0) { errno=ENOSPC; return EOF; } return rc;
}
static int test_mkdir(const char *p,int mode) { (void)mode; return _mkdir(p); }
#ifndef S_ISDIR
#define S_ISDIR(m) (((m)&S_IFMT)==S_IFDIR)
#endif
#define mkdir test_mkdir
#define fwrite injected_fwrite
#define fflush injected_fflush
#define fclose injected_fclose
#include "../src/clip_writer.c"
#undef fwrite
#undef fflush
#undef fclose
#undef mkdir
#include "../src/ring_buffer.c"
#include "../src/config.c"
int event_queue_pop(EventQueue *q,AbnormalEvent *e) { (void)q;(void)e;return CA_ERR; }
int upload_queue_push(UploadQueue *q,const UploadJob *j,int b) { (void)q;(void)j;(void)b;return CA_ERR; }
static int exists(const char *p) { FILE *f=fopen(p,"rb"); if(!f)return 0;fclose(f);return 1; }
static void create(const char *p) { FILE *f=event_file_create(p);assert(f);fputs("keep",f);assert(fclose(f)==0); }
static void keep_content(const char *p) { char b[5]={0};FILE *f=fopen(p,"rb");assert(f);assert(fread(b,1,4,f)==4);fclose(f);assert(!strcmp(b,"keep")); }
int main(void) {
    const char *v="cleanup_test.h264", *m="cleanup_test.json";
    PacketRing r; AppConfig cfg; AbnormalEvent ev={0}; EncodedPacket p={0};
    unsigned char bytes[8]={0,0,0,1,0x65,1,2,3}; FILE *f;
    assert(!exists(v)&&!exists(m));
    create(v);create(m);
    event_files_after_upload(0,1,v,m);assert(exists(v)&&exists(m));
    event_files_after_upload(1,0,v,m);assert(exists(v)&&exists(m));
    assert(!event_file_create(v));keep_content(v);
    event_files_after_upload(1,1,v,m);assert(!exists(v)&&!exists(m));
    event_files_after_upload(1,1,v,m); /* already missing is harmless */
    config_defaults(&cfg);assert(cfg.delete_after_upload==1);
    assert(ring_init(&r,10,20,1024)==CA_OK);
    p.data=bytes;p.size=8;p.codec=CODEC_H264;p.recv_ms=1000;p.key_frame=1;
    fail_write=1;assert(write_clip_with_params(&r,v,m,&cfg,&ev,&p,1,0)==CA_ERR);
    assert(!exists(v)&&!exists(m));
    fail_close=1;assert(write_clip_with_params(&r,v,m,&cfg,&ev,&p,1,0)==CA_ERR);
    assert(!exists(v)&&!exists(m));
    fail_flush=1;assert(write_clip_with_params(&r,v,m,&cfg,&ev,&p,1,0)==CA_ERR);
    assert(!exists(v)&&!exists(m));
    fail_close=2;assert(write_clip_with_params(&r,v,m,&cfg,&ev,&p,1,0)==CA_ERR);
    assert(!exists(v)&&!exists(m));
    create(m);assert(write_clip_with_params(&r,v,m,&cfg,&ev,&p,1,0)==CA_ERR);
    assert(!exists(v));keep_content(m);event_file_delete(m,"test");
    create(v);assert(write_clip_with_params(&r,v,m,&cfg,&ev,&p,1,0)==CA_ERR);
    keep_content(v);assert(!exists(m));event_file_delete(v,"test");
    assert(write_clip_with_params(&r,v,m,&cfg,&ev,&p,1,0)==CA_OK);
    assert(exists(v)&&exists(m));event_files_after_upload(1,1,v,m);
    ring_destroy(&r);
    f=fopen("cleanup_config.tmp","w");assert(f);fputs("delete_after_upload=0\n",f);fclose(f);
    assert(config_load("cleanup_config.tmp",&cfg)==CA_OK && !cfg.delete_after_upload);
    f=fopen("cleanup_config.tmp","w");assert(f);fputs("delete_after_upload=yes\n",f);fclose(f);
    assert(config_load("cleanup_config.tmp",&cfg)==CA_ERR);unlink("cleanup_config.tmp");
    puts("PASS: successful upload deletion, failed/disabled retention, missing files, write/flush/close failure cleanup, collision protection, config");
    return 0;
}
