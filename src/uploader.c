#include "uploader.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

typedef struct {
    char host[128];
    int port;
    char path[256];
} HttpUrl;

static int parse_http_url(const char *url, HttpUrl *out)
{
    const char *p;
    const char *slash;
    const char *colon;
    int host_len;
    memset(out, 0, sizeof(*out));
    out->port = 80;
    if (strncmp(url, "http://", 7) != 0) {
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

static int queue_common_init(UploadQueue *q, int capacity)
{
    memset(q, 0, sizeof(*q));
    q->items = (UploadJob *)calloc((size_t)capacity, sizeof(UploadJob));
    if (!q->items) {
        return CA_ERR;
    }
    q->capacity = capacity;
    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->not_empty, NULL);
    pthread_cond_init(&q->not_full, NULL);
    return CA_OK;
}

int upload_queue_init(UploadQueue *q, int capacity)
{
    return queue_common_init(q, capacity);
}

void upload_queue_destroy(UploadQueue *q)
{
    free(q->items);
    pthread_mutex_destroy(&q->mutex);
    pthread_cond_destroy(&q->not_empty);
    pthread_cond_destroy(&q->not_full);
    memset(q, 0, sizeof(*q));
}

void upload_queue_close(UploadQueue *q)
{
    pthread_mutex_lock(&q->mutex);
    q->closed = 1;
    pthread_cond_broadcast(&q->not_empty);
    pthread_cond_broadcast(&q->not_full);
    pthread_mutex_unlock(&q->mutex);
}

int upload_queue_push(UploadQueue *q, const UploadJob *job, int block)
{
    pthread_mutex_lock(&q->mutex);
    while (!q->closed && q->count == q->capacity && block) {
        pthread_cond_wait(&q->not_full, &q->mutex);
    }
    if (q->closed || q->count == q->capacity) {
        int closed = q->closed;
        int count = q->count;
        int capacity = q->capacity;
        ca_debug_log(1, "upload_queue_push rejected: closed=%d count=%d capacity=%d block=%d",
                     closed, count, capacity, block);
        pthread_mutex_unlock(&q->mutex);
        return CA_ERR;
    }
    q->items[q->tail] = *job;
    q->tail = (q->tail + 1) % q->capacity;
    q->count++;
    ca_debug_log(2, "upload_queue_push: video=%s count=%d/%d",
                 job->video_path, q->count, q->capacity);
    pthread_cond_signal(&q->not_empty);
    pthread_mutex_unlock(&q->mutex);
    return CA_OK;
}

int upload_queue_pop(UploadQueue *q, UploadJob *job)
{
    pthread_mutex_lock(&q->mutex);
    while (!q->closed && q->count == 0) {
        pthread_cond_wait(&q->not_empty, &q->mutex);
    }
    if (q->count == 0 && q->closed) {
        pthread_mutex_unlock(&q->mutex);
        return CA_ERR;
    }
    *job = q->items[q->head];
    q->head = (q->head + 1) % q->capacity;
    q->count--;
    ca_debug_log(2, "upload_queue_pop: video=%s count=%d/%d",
                 job->video_path, q->count, q->capacity);
    pthread_cond_signal(&q->not_full);
    pthread_mutex_unlock(&q->mutex);
    return CA_OK;
}

static int tcp_connect_http(const char *host, int port)
{
    int fd;
    struct sockaddr_in addr;
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        return -1;
    }
    {
        struct timeval tv;
        tv.tv_sec = 5;
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
        ca_log("ERR", "upload connect failed: %s:%d errno=%d (%s)",
               host, port, errno, strerror(errno));
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

static long file_size(const char *path)
{
    struct stat st;
    if (stat(path, &st) != 0) {
        return -1;
    }
    return (long)st.st_size;
}

static int send_file(int fd, const char *path)
{
    FILE *fp = fopen(path, "rb");
    uint8_t buf[8192];
    if (!fp) {
        return CA_ERR;
    }
    for (;;) {
        size_t n = fread(buf, 1, sizeof(buf), fp);
        if (n > 0 && send_all(fd, buf, (int)n) != CA_OK) {
            fclose(fp);
            return CA_ERR;
        }
        if (n < sizeof(buf)) {
            if (ferror(fp)) {
                fclose(fp);
                return CA_ERR;
            }
            break;
        }
    }
    fclose(fp);
    return CA_OK;
}

static int read_status(int fd)
{
    char buf[512];
    int n = (int)recv(fd, buf, sizeof(buf) - 1, 0);
    int code = 0;
    if (n <= 0) {
        ca_log("ERR", "upload response read failed");
        return CA_ERR;
    }
    buf[n] = '\0';
    if (sscanf(buf, "HTTP/%*s %d", &code) != 1) {
        ca_log("ERR", "upload response parse failed: %.120s", buf);
        return CA_ERR;
    }
    ca_debug_log(1, "upload HTTP status: %d", code);
    return (code >= 200 && code < 300) ? CA_OK : CA_ERR;
}

static int upload_once(const AppConfig *cfg, const UploadJob *job)
{
    HttpUrl url;
    int fd;
    char boundary[96];
    char header[1024];
    char part1[1024];
    char part2[1024];
    char end[128];
    long video_len;
    long meta_len;
    long total_len;
    const char *ext = job->codec == CODEC_H265 ? "h265" : "h264";
    if (parse_http_url(cfg->upload_url, &url) != CA_OK) {
        ca_log("ERR", "only http://IPv4:port/path upload_url is supported now");
        return CA_ERR;
    }
    video_len = file_size(job->video_path);
    meta_len = file_size(job->meta_path);
    if (video_len <= 0 || meta_len <= 0) {
        ca_log("ERR", "upload file invalid: video_len=%ld meta_len=%ld video=%s meta=%s",
               video_len, meta_len, job->video_path, job->meta_path);
        return CA_ERR;
    }
    ca_debug_log(1, "upload_once: url=%s:%d%s video_len=%ld meta_len=%ld codec=%s",
                 url.host, url.port, url.path, video_len, meta_len, ext);
    snprintf(boundary, sizeof(boundary), "----camera-abnormal-%lld", (long long)ca_now_ms());
    snprintf(part1, sizeof(part1),
             "--%s\r\nContent-Disposition: form-data; name=\"metadata\"; filename=\"event.json\"\r\n"
             "Content-Type: application/json\r\n\r\n", boundary);
    snprintf(part2, sizeof(part2),
             "\r\n--%s\r\nContent-Disposition: form-data; name=\"video\"; filename=\"event.%s\"\r\n"
             "Content-Type: application/octet-stream\r\n\r\n", boundary, ext);
    snprintf(end, sizeof(end), "\r\n--%s--\r\n", boundary);
    total_len = (long)strlen(part1) + meta_len + (long)strlen(part2) + video_len + (long)strlen(end);
    snprintf(header, sizeof(header),
             "POST %s HTTP/1.1\r\nHost: %s:%d\r\nConnection: close\r\n"
             "Content-Type: multipart/form-data; boundary=%s\r\nContent-Length: %ld\r\n\r\n",
             url.path, url.host, url.port, boundary, total_len);
    fd = tcp_connect_http(url.host, url.port);
    if (fd < 0) {
        ca_log("ERR", "connect upload server failed: %s:%d", url.host, url.port);
        return CA_ERR;
    }
    if (send_all(fd, header, (int)strlen(header)) != CA_OK ||
        send_all(fd, part1, (int)strlen(part1)) != CA_OK ||
        send_file(fd, job->meta_path) != CA_OK ||
        send_all(fd, part2, (int)strlen(part2)) != CA_OK ||
        send_file(fd, job->video_path) != CA_OK ||
        send_all(fd, end, (int)strlen(end)) != CA_OK) {
        close(fd);
        return CA_ERR;
    }
    if (read_status(fd) != CA_OK) {
        close(fd);
        return CA_ERR;
    }
    close(fd);
    return CA_OK;
}

void *upload_thread(void *arg)
{
    UploadContext *ctx = (UploadContext *)arg;
    UploadJob job;
    while (*ctx->running && upload_queue_pop(ctx->queue, &job) == CA_OK) {
        int i;
        int ok = 0;
        for (i = 0; i < ctx->cfg->upload_retry && *ctx->running; i++) {
            if (upload_once(ctx->cfg, &job) == CA_OK) {
                ok = 1;
                break;
            }
            ca_log("WARN", "upload failed, retry %d/%d: %s", i + 1, ctx->cfg->upload_retry, job.video_path);
            ca_sleep_ms(ctx->cfg->upload_retry_interval_ms);
        }
        ca_log(ok ? "INFO" : "ERR", "%s upload: %s", ok ? "confirmed" : "failed", job.video_path);
    }
    return NULL;
}
