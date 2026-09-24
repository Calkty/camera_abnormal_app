# Storage and detection settings

## Server storage

Edit server/server_config.json, for example:

```json
{"upload_dir": "D:/camera_videos"}
```

Use forward slashes in Windows JSON paths. The directory is created at startup.
Raw video, converted MP4 and event JSON all use this directory.
Restart the server after editing. Existing files are not moved.
Relative paths are resolved against the server directory, not the launch directory.
The existing CAMERA_UPLOAD_DIR environment variable takes precedence over JSON.
The resolved location is printed as [STORAGE] at startup.

For Docker Compose, set CAMERA_UPLOAD_HOST_DIR in server/.env, for example:

```text
CAMERA_UPLOAD_HOST_DIR=D:/camera_videos
```

Then recreate the service with docker compose up -d --build.
The container still uses /data/uploads; the host directory is configured by the
volume mapping, not by a Windows path inside the container.

## Detection threshold

In app.conf:

```ini
confidence_threshold=0.5
```

Accepted range: 0 to 1 inclusive. Missing setting defaults to 0.0 to preserve
previous behavior. Invalid values (including NaN, infinity or trailing text)
reject the configuration. Scores below the threshold are discarded before
target display and alarm selection; scores equal to it pass this check.
Class and region filters still apply. This setting affects CAMERA inference,
not the offline FILE-mode model test. It does not change model-internal NMS
or recover detections already discarded inside the model.

Rebuild/reinstall the camera application for this code change. Subsequent
threshold edits require an application restart. makeapp.sh copies app.conf
into APP during packaging. The selected threshold is logged at startup.

New camera builds report the raw model score of the first target passing
confidence, class and region filters (the same target selected for the alarm).
This is not necessarily the highest score when multiple targets are present.
The event JSON retains two decimal places. Existing records and older camera
builds may still contain the previous fixed 1.0; those cannot be corrected
retroactively. Restart the server to remove its obsolete fixed-score notice.
