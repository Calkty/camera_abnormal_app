/* Compile with tests/compat only for Windows local tests, not production. */
#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
int64_t test_now_ms = 1000;
int g_ca_debug_level = 0;
static int fail_next_malloc;
static void *test_malloc(size_t n) {
    if (fail_next_malloc) { fail_next_malloc=0; return NULL; }
    return malloc(n);
}
#define malloc test_malloc
#include "../src/ring_buffer.c"
#undef malloc
#include "../src/config.c"

static unsigned char data[1024];
static void push(PacketRing *r, int n, int64_t wall) {
    assert(ring_push(r, data, n, 0, wall, 0, 0, CODEC_H264)==CA_OK);
}
static void invariant(PacketRing *r) {
    int i; size_t sum=0;
    assert(r->count>=0 && r->count<=r->capacity);
    for(i=0;i<r->count;i++) {
        EncodedPacket *p=&r->pkts[(r->head+i)%r->capacity];
        assert(p->data && p->size>0); sum+=(size_t)p->size;
    }
    assert(sum==r->payload_bytes && sum<=r->max_bytes);
}
int main(void) {
    PacketRing r; EncodedPacket *snap=NULL; int count=0, i, seconds;
    AppConfig cfg; FILE *fp;
    assert(ring_init(&r,0,20,1000)==CA_ERR);
    assert(ring_init(&r,100,20,1000)==CA_OK);
    push(&r,600,1000); push(&r,400,1000); invariant(&r);
    assert(r.payload_bytes==1000);
    push(&r,100,1000); invariant(&r);
    assert(r.count==2 && r.payload_bytes==500 && r.evicted_bytes==1);
    assert(ring_push(&r,data,1001,0,1000,0,0,CODEC_H264)==CA_ERR);
    assert(r.dropped_oversize==1 && r.payload_bytes==500);
    fail_next_malloc=1;
    assert(ring_push(&r,data,10,0,1000,0,0,CODEC_H264)==CA_ERR);
    invariant(&r); assert(r.count==2 && r.alloc_failures==1);
    assert(ring_snapshot(&r,&snap,&count)==CA_OK && count==2);
    test_now_ms+=20000; ring_prune(&r);
    assert(r.count==0 && r.payload_bytes==0 && r.evicted_time==2);
    assert(snap[0].data && snap[0].size==400); packet_array_free(snap,count);
    assert(ring_snapshot(&r,&snap,&count)==CA_OK && !snap && !count);
    ring_destroy(&r);
    /* Metadata wraparound and full-queue allocation failure recovery. */
    assert(ring_init(&r,2,20,1000)==CA_OK);
    push(&r,10,0); push(&r,20,1); push(&r,30,2);
    assert(r.count==2 && r.payload_bytes==50 && r.evicted_count==1);
    fail_next_malloc=1;
    assert(ring_push(&r,data,40,0,3,0,0,CODEC_H264)==CA_ERR);
    invariant(&r); assert(r.count==1 && r.payload_bytes==30);
    push(&r,40,4); invariant(&r); ring_destroy(&r);
    /* Sustained byte pressure: limit wins even when time/count limits are distant. */
    assert(ring_init(&r,4096,20,1000)==CA_OK);
    for(i=0;i<10000;i++) {
        test_now_ms+=1; push(&r,600,1234); invariant(&r);
        assert(r.count==1 && r.payload_bytes==600);
    }
    assert(r.evicted_bytes==9999 && r.peak_payload_bytes==600);
    ring_destroy(&r);
    /* 15 minutes at 25 samples/sec: 10s and 20s stop growing, despite wall jumps. */
    for(seconds=10;seconds<=20;seconds+=10) {
        assert(ring_init(&r,4096,seconds,16*1024*1024)==CA_OK);
        for(i=0;i<22500;i++) {
            test_now_ms+=40; push(&r,1000, i%2 ? 9000000 : 100);
            invariant(&r);
            assert(r.count<=seconds*25);
        }
        assert(r.count==seconds*25 && r.evicted_time>0);
        test_now_ms+=seconds*1000; ring_prune(&r); assert(r.count==0);
        ring_destroy(&r);
    }
    config_defaults(&cfg); assert(cfg.ring_seconds==20 && cfg.ring_max_mb==16);
    fp=fopen("test_config.tmp","w"); assert(fp);
    fputs("ring_seconds=10\nring_max_mb=8\n",fp); fclose(fp);
    assert(config_load("test_config.tmp",&cfg)==CA_OK);
    assert(cfg.ring_seconds==10 && cfg.ring_max_mb==8);
    fp=fopen("test_config.tmp","w"); assert(fp);
    fputs("ring_max_mb=-1\n",fp); fclose(fp);
    assert(config_load("test_config.tmp",&cfg)==CA_ERR);
    remove("test_config.tmp");
    puts("PASS: byte/time/count eviction, idle expiry, 10s/20s soak, wall jumps, oversize, malloc failure, snapshot ownership, config");
    return 0;
}
