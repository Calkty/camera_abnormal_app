#ifndef CAMERA_FAILED_ARCHIVE_H
#define CAMERA_FAILED_ARCHIVE_H

#include "event_files.h"
#include <mntent.h>
#include <sys/stat.h>

#define CA_SD_MOUNT "/mnt/mmc01"

/* Refuse a plain internal-storage directory when the card is not mounted. */
static int failed_archive_mount_present(void)
{
    FILE *fp = setmntent("/proc/mounts", "r");
    struct mntent *entry;
    int found = 0;
    if (!fp) return 0;
    while ((entry = getmntent(fp)) != NULL) {
        if (!strcmp(entry->mnt_dir, CA_SD_MOUNT) ||
            !strcmp(entry->mnt_dir, CA_SD_MOUNT "/")) {
            found = 1;
            break;
        }
    }
    endmntent(fp);
    return found;
}

static int failed_archive_copy(int dirfd, const char *source)
{
    const char *name = strrchr(source, '/');
    int in = -1, out = -1, rc = CA_ERR, saved;
    char buffer[32768];
    ssize_t n;
    struct stat st;
    name = name ? name + 1 : source;
    in = open(source, O_RDONLY | O_NOFOLLOW | O_NONBLOCK | O_CLOEXEC);
    if (in < 0) goto done;
    if (fstat(in, &st) != 0) goto done;
    if (!S_ISREG(st.st_mode)) { errno = EINVAL; goto done; }
    out = openat(dirfd, name, O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, 0644);
    if (out < 0) goto done;
    for (;;) {
        n = read(in, buffer, sizeof(buffer));
        if (n < 0 && errno == EINTR) continue;
        if (n < 0) goto done;
        if (n == 0) break;
        ssize_t offset = 0;
        while (offset < n) {
            ssize_t written = write(out, buffer + offset, (size_t)(n - offset));
            if (written < 0 && errno == EINTR) continue;
            if (written <= 0) { if (!written) errno = EIO; goto done; }
            offset += written;
        }
    }
    if (fsync(out) != 0) goto done;
    if (close(out) != 0) {
        out = -1;
        saved = errno;
        unlinkat(dirfd, name, 0);
        errno = saved;
        goto done;
    }
    out = -1;
    rc = CA_OK;
done:
    saved = errno;
    if (out >= 0) {
        close(out);
        unlinkat(dirfd, name, 0);
    }
    if (in >= 0) close(in);
    if (rc != CA_OK)
        ca_log("ERR", "SD archive copy failed: source=%s errno=%d (%s)",
               source, saved, strerror(saved));
    return rc;
}

/* Upload failure is terminal for this job; local files are always removed.
 * Each job gets an exclusive directory, never overwriting older SD evidence. */
static void failed_archive_and_remove(const char *video, const char *meta)
{
    static unsigned long serial;
    int root = -1, dir = -1, saved_video = 0, saved_meta = 0, created = 0;
    char name[96];
    if (!failed_archive_mount_present()) {
        ca_log("ERR", "SD archive unavailable: %s is not mounted", CA_SD_MOUNT);
        goto done;
    }
    root = open(CA_SD_MOUNT, O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
    if (root < 0) goto error;
    snprintf(name, sizeof(name), "cameraAbnormal_failed_%lld_%ld_%lu",
             (long long)ca_now_ms(), (long)getpid(), ++serial);
    if (mkdirat(root, name, 0755) != 0) goto error;
    created = 1;
    dir = openat(root, name, O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
    if (dir < 0) goto error;
    saved_video = failed_archive_copy(dir, video) == CA_OK;
    if (saved_video) saved_meta = failed_archive_copy(dir, meta) == CA_OK;
    if (fsync(dir) != 0 || fsync(root) != 0) goto error;
    ca_log(saved_video && saved_meta ? "INFO" : "ERR",
           "SD archive result: dir=%s/%s video=%d metadata=%d",
           CA_SD_MOUNT, name, saved_video, saved_meta);
    goto done;
error:
    ca_log("ERR", "SD archive failed: errno=%d (%s)", errno, strerror(errno));
done:
    if (dir >= 0) close(dir);
    if (created && !saved_video) unlinkat(root, name, AT_REMOVEDIR);
    if (root >= 0) close(root);
    event_file_delete(video, "upload_failed_sd_attempted");
    event_file_delete(meta, "upload_failed_sd_attempted");
}
#endif
