#ifndef CAMERA_ABNORMAL_EVENT_FILES_H
#define CAMERA_ABNORMAL_EVENT_FILES_H
#include "common.h"
#include <fcntl.h>
#include <unistd.h>

/* Only exact paths owned by the current job; never scan/delete a directory. */
static inline int event_file_delete(const char *path, const char *reason)
{
    int saved;
    if (!path || !path[0]) return CA_OK;
    if (unlink(path) == 0) {
        ca_log("INFO", "local file deleted: reason=%s path=%s", reason, path);
        return CA_OK;
    }
    saved = errno;
    if (saved == ENOENT) return CA_OK;
    ca_log("ERR", "local file delete failed: reason=%s path=%s errno=%d (%s)",
           reason, path, saved, strerror(saved));
    return CA_ERR;
}

static inline FILE *event_file_create(const char *path)
{
    int flags = O_WRONLY | O_CREAT | O_EXCL;
    int fd, saved;
    FILE *fp;
#ifdef O_BINARY
    flags |= O_BINARY;
#endif
    /* Do not truncate/delete an older or queued event with the same name. */
    fd = open(path, flags, 0644);
    if (fd < 0) return NULL;
    fp = fdopen(fd, "wb");
    if (!fp) {
        saved = errno;
        close(fd);
        event_file_delete(path, "stream_open_failed");
        errno = saved;
    }
    return fp;
}

static inline void event_files_after_upload(int confirmed, int delete_enabled,
                                            const char *video, const char *meta)
{
    if (!confirmed || !delete_enabled) return;
    /* Keep metadata if video deletion fails, for later manual reconciliation. */
    if (event_file_delete(video, "upload_confirmed") == CA_OK)
        event_file_delete(meta, "upload_confirmed");
}
#endif
