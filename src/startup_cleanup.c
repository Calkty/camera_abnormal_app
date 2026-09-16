#include "startup_cleanup.h"
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#define CLEAR_WORK_DIR "/heop/package/cameraAbnormal/user_data/camera_abnormal"
#define CLEAR_MARKER ".events_clear_once_v1.done"

static int event_suffix(const char *name)
{
    const char *ext = strrchr(name, '.');
    return ext && (!strcmp(ext, ".h264") || !strcmp(ext, ".h265") || !strcmp(ext, ".json"));
}

/* No shell commands, no recursive deletion, no traversal through symlinks. */
int clear_events_once(const AppConfig *cfg)
{
    int parent = -1, fd = -1, marker = -1, rc = CA_ERR, saved;
    DIR *dir = NULL;
    struct dirent *entry;
    struct stat st;
    unsigned long removed = 0, skipped = 0;
    char token[6];
    if (!cfg->clear_events_once) return CA_OK;
    /* This one-shot release is authorized for this exact device directory only. */
    if (strcmp(cfg->work_dir, CLEAR_WORK_DIR)) {
        ca_log("ERR", "CLEAR_ONCE refused: work_dir must be %s", CLEAR_WORK_DIR);
        return CA_ERR;
    }
    parent = open(CLEAR_WORK_DIR, O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
    if (parent < 0) goto error;
    marker = openat(parent, CLEAR_MARKER, O_RDONLY | O_NOFOLLOW | O_NONBLOCK | O_CLOEXEC);
    if (marker >= 0) {
        if (fstat(marker, &st) != 0 || !S_ISREG(st.st_mode) || st.st_size != 5 ||
            read(marker, token, sizeof(token)) != 5 || memcmp(token, "done\n", 5)) {
            ca_log("ERR", "CLEAR_ONCE invalid marker; refusing to delete again");
            goto done;
        }
        ca_log("INFO", "CLEAR_ONCE already completed; keeping current event files");
        rc = CA_OK;
        goto done;
    }
    if (errno != ENOENT) goto error;
    fd = openat(parent, "events", O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
    if (fd < 0) goto error;
    dir = fdopendir(fd);
    if (!dir) goto error;
    ca_log("INFO", "CLEAR_ONCE begin: %s/events (old video and JSON, including unuploaded)", CLEAR_WORK_DIR);
    for (;;) {
        errno = 0;
        entry = readdir(dir);
        if (!entry) {
            if (errno) goto error;
            break;
        }
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")) continue;
        if (!event_suffix(entry->d_name)) { ++skipped; continue; }
        if (fstatat(fd, entry->d_name, &st, AT_SYMLINK_NOFOLLOW) != 0) {
            if (errno == ENOENT) continue;
            goto error;
        }
        /* Directories and symlinks, even with matching extensions, are untouched. */
        if (!S_ISREG(st.st_mode)) { ++skipped; continue; }
        if (unlinkat(fd, entry->d_name, 0) != 0) {
            if (errno == ENOENT) continue;
            ca_log("ERR", "CLEAR_ONCE delete failed: %s errno=%d (%s)",
                   entry->d_name, errno, strerror(errno));
            goto done;
        }
        ++removed;
    }
    /* Commit deletes before recording completion; do not start producers on failure. */
    if (fsync(fd) != 0) goto error;
    marker = openat(parent, CLEAR_MARKER, O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, 0644);
    if (marker < 0) goto error;
    if (write(marker, "done\n", 5) != 5 || fsync(marker) != 0) {
        saved = errno;
        close(marker); marker = -1;
        unlinkat(parent, CLEAR_MARKER, 0);
        errno = saved;
        goto error;
    }
    if (close(marker) != 0) { marker = -1; goto error; }
    marker = -1;
    if (fsync(parent) != 0) goto error;
    ca_log("INFO", "CLEAR_ONCE complete: deleted=%lu skipped=%lu marker=%s/%s",
           removed, skipped, CLEAR_WORK_DIR, CLEAR_MARKER);
    rc = CA_OK;
    goto done;
error:
    saved = errno;
    ca_log("ERR", "CLEAR_ONCE failed: errno=%d (%s), deleted=%lu; application will not start",
           saved, strerror(saved), removed);
done:
    if (marker >= 0) close(marker);
    if (dir) closedir(dir); else if (fd >= 0) close(fd);
    if (parent >= 0) close(parent);
    return rc;
}
