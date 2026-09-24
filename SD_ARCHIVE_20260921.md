# 2026-09-21 storage and overlay update

## One-time cleanup

Keep clear_events_once=1 in app.conf. On the first successful startup of this
build, the existing cleanup routine removes regular .h264/.h265/.json files
directly inside:

/heop/package/cameraAbnormal/user_data/camera_abnormal/events

This includes unuploaded files, as requested. Other files, directories and
symlinks are not deleted. The new completion marker is
.events_clear_once_20260921.done in the parent work directory. The old v1
marker does not suppress this new cleanup. Subsequent starts skip cleanup.
Do not remove the completion marker unless intentionally requesting another
cleanup. A failed/interrupted cleanup may resume on next startup.

No device-side cleanup has been executed from the development computer.
Install and start the newly built application to execute it.

## Upload failure

After upload retries are exhausted, the application checks /proc/mounts for
/mnt/mmc01, opens the mounted directory, and copies video and event JSON into
an exclusive cameraAbnormal_failed_<time>_<pid>_<serial> directory on the card.
Copies use a bounded buffer and fsync. A failed partial copy is removed.
If video succeeds but JSON fails, the complete video is retained.

After the attempt, both source files in the internal events directory are
deleted even if the card is full, absent, read-only or inaccessible. Deletion
errors are logged. An unavailable card is never replaced by an automatically
created internal-storage directory. The next failed job attempts SD storage
again. SD archives are not automatically re-uploaded or deleted.

The HEOP application container must expose the mounted SD path. Otherwise
the log reports "SD archive unavailable" and the failed clip is discarded.
Check device permissions/mount visibility during deployment.

Successful uploads still use delete_after_upload=1. This change does not
redirect clips when ENABLE_UPLOAD=0, nor restore pending tasks after a crash.

## Web overlay

Each accepted detection carries its own model class ID and raw confidence
through the snapshot and detections JSON. Labels show:

class:4 confidence:0.82

The class ID is the model output, not the per-frame target ID. Names from
hikflow_attr.json remain in the API for compatibility but are no longer used
as the primary browser label. Scores are displayed to two decimal places;
threshold checks use original precision. Each target retains its own score.

## Deployment and checks

Rebuild using makeapp.sh with the desired module switches, install the new
.app and hard-refresh the application page. For upload failure testing,
enable INFER/RING/CLIP/UPLOAD and disable LEGACY_ALARM if not needed.

Check CLEAR_ONCE complete on first start and already completed on restart.
For failed uploads check SD archive result and upload_failed_sd_attempted
deletion logs. Verify SD absence/full-card failures do not prevent later jobs.
Check detections JSON has cls and confidence for every box.
