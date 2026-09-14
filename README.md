# camera_abnormal_app

This project is the first integrated implementation for the ball-camera abnormal-event pipeline:

1. `main` initializes config, signals, queues and worker threads.
2. `infer_thread` starts the vendored `human_detect_demo_v2` detection stack: BSC/VIN -> mscale -> HIKFlow -> abnormal event.
3. `rtsp_record_thread` self-pulls the camera RTSP main stream through RTP over TCP and writes H264/H265 Annex-B NALU packets into an in-memory timestamped ring buffer.
4. `event_clip_thread` copies pre-event packets immediately, waits for post-event packets, key-frame aligns the output, injects SPS/PPS/VPS, and writes `.h264` or `.h265`.
5. `upload_thread` uploads metadata and video through HTTP multipart/form-data with retry and 2xx confirmation.
6. The external server receives the raw stream and converts it to MP4 with `ffmpeg`.

## Build On HEOP Container

Copy this directory into the HEOP build container, then run:

```sh
make clean
make CC=aarch64-mix210-linux-gcc
make install
```

The result is copied into `APP/`:

```text
APP/camera_abnormal_app
APP/app.conf
APP/cameraAbnormal.sh
APP/META-INFO/MANIFEST.MF
APP/html/index.html
```

If your HEOP SDK layout differs, append extra libraries through `EXTRA_LIBS`, for example:

```sh
make CC=aarch64-mix210-linux-gcc EXTRA_LIBS="-lxxx"
```

The upload implementation uses raw sockets and does not depend on `libcurl`.
The vendored `human_detect_demo_v2` code still links against the same HEOP SDK
libraries as the original demo.

## Camera Config

Edit `app.conf` before packaging or deployment:

```ini
camera_id=ptz_001
rtsp_url=rtsp://127.0.0.1:554/ISAPI/Streaming/channels/101
upload_url=http://192.168.1.100:8080/api/upload
work_dir=/tmp/camera_abnormal
hikflow_model_path=
abnormal_classes=1,2,3
human_alarm_ip=10.184.142.15
human_alarm_port=7200
listen_port=0
app_chan=-1
pre_seconds=5
post_seconds=10
ring_seconds=30
max_events=4
upload_retry=3
upload_retry_interval_ms=3000
cooldown_seconds=20
infer_interval_seconds=0
codec=auto
fps=25
debug_level=1
test_event_interval_seconds=0
```

Recommended sizing:

```text
ring_seconds >= pre_seconds + post_seconds + max_detection_latency_seconds + 5
```

For example, `pre_seconds=5`, `post_seconds=10`, model delay under 1 second, use `ring_seconds=30`.

For pipeline testing without the model, set:

```ini
test_event_interval_seconds=30
```

This periodically creates abnormal events so you can verify RTSP recording, clipping and upload.

`debug_level` controls extra runtime diagnostics. `0` disables debug logs; `1`
prints key checkpoints such as configuration, RTSP receive statistics, abnormal
event bridge, clip window and upload status; `2` also prints queue push/pop and
ring-buffer overwrite/snapshot details.

`infer_interval_seconds` controls how often the HIKFlow model is executed in
camera mode. `0` keeps the original behavior and runs inference on every
available `net_frame`; `1` means at most once per second; `5` means at most once
every five seconds. VIN, mscale, RTSP recording and ring-buffer caching continue
to run continuously.

`upload_url` is the final event clip upload endpoint. `human_alarm_ip` and
`human_alarm_port` configure the legacy `human_detect_demo_v2` alarm sender in
`vendor/human_detect_demo_v2/src/alarm/alarm.c`, which sends the original
JSON/JPEG alarm message.

`hikflow_model_path` optionally overrides `hikflow_config.json`'s `model_path`.
Leave it empty to keep the original JSON-driven model path. `abnormal_classes`
is a comma-separated class list; for a four-class model where class `0` means
normal and `1,2,3` mean abnormal, use `abnormal_classes=1,2,3`.

## HIKFlow Integration Point

This project now includes the original detection-related `human_detect_demo_v2`
source tree under `vendor/human_detect_demo_v2`. The final application starts
that detection stack from `src/infer_adapter.c`.

The runtime detection flow is:

```text
BSC/VIN frame acquisition
-> mscale preprocessing
-> HIKFlow inference
-> original hikflow_demo_proc_alarm()
-> event_bridge callback
-> abnormal_event_publish(...)
```

Do not decode the RTSP stream for inference in this architecture. RTSP is only used for encoded clip recording.
See `PROJECT_RELATIONSHIP.md` for the mapping between original projects and the
final project modules.

## External Server

On the external Linux server:

```sh
cd server
docker compose up -d --build
```

Uploaded files appear under:

```text
server/uploads/<event_timestamp>/
  event.json
  event.h264 or event.h265
  event.mp4
```

Without Docker:

```sh
cd server
python3 -m venv .venv
. .venv/bin/activate
pip install -r requirements.txt
sudo apt-get install -y ffmpeg
CAMERA_UPLOAD_DIR=uploads uvicorn server:app --host 0.0.0.0 --port 8080
```

## Current Implementation Coverage

Implemented:

- Four-thread application structure.
- RTSP DESCRIBE/SETUP/PLAY over TCP.
- RTP interleaved parsing.
- H264 single NALU, STAP-A, FU-A.
- H265 single NALU, AP, FU.
- Timestamped in-memory Annex-B ring buffer.
- Event pre/post clipping.
- Key-frame start alignment.
- SPS/PPS/VPS injection before written clip.
- HTTP multipart upload without curl.
- Upload retry and server 2xx confirmation.
- Server-side MP4 conversion.

Still hardware-specific:

- This project still requires the HEOP SDK headers and device libraries under `/heop/include` and `/heop/lib`.
- The original detection source is vendored under `vendor/human_detect_demo_v2`; it is no longer expected to be available as a separate project at `/heop/demo/human_detect_demo_v2`.





