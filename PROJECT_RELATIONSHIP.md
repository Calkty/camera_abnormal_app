# Source Relationship

This document records which original projects were used and where their roles
live in the final `camera_abnormal_app` project.

## Final Project

```text
camera_abnormal_app/
```

This is now intended to be the main ball-camera application. It owns:

```text
main thread
infer_thread
rtsp_record_thread
event_clip_thread
upload_thread
```

## human_detect_demo_v2

Original role:

```text
BSC/VIN frame acquisition
mscale preprocessing
HIKFlow model loading and inference
human alarm/rule processing
ISAPI configuration callbacks
audio/alarm linkage
```

The original `src/alarm/alarm.c` hardcoded its HTTP alarm server as
`10.184.142.15:7200`. In this final project, that target is configured through
`app.conf` as `human_alarm_ip` and `human_alarm_port`. This is separate from
`upload_url`, which is used by `src/uploader.c` for abnormal video clip upload.

Current location in final project:

```text
vendor/human_detect_demo_v2/
```

Migrated source groups:

```text
vendor/human_detect_demo_v2/src/alarm
vendor/human_detect_demo_v2/src/audioplay
vendor/human_detect_demo_v2/src/config
vendor/human_detect_demo_v2/src/deps
vendor/human_detect_demo_v2/src/hikflow
vendor/human_detect_demo_v2/src/protocol
```

Not built:

```text
human_detect_demo_v2/src/main.c
```

Reason: `camera_abnormal_app/src/main.c` is the final application's only main
entry point.

Bridge point:

```text
vendor/human_detect_demo_v2/src/hikflow/code/hikflow_demo.c
  hikflow_demo_proc_alarm()
    if (1 == alarm_flag)
      camera_abnormal_on_human_alarm(...)
```

The bridge publishes into:

```text
camera_abnormal_app/src/infer_adapter.c
camera_abnormal_app/src/event_queue.c
```

## rtsp_demo_v2

Original role:

```text
RTSP self-pull without external authentication
DESCRIBE/SETUP/PLAY request construction
RTP over TCP interleaved receiving
```

Current implementation in final project:

```text
camera_abnormal_app/src/rtsp_client.c
camera_abnormal_app/src/rtp_h26x.c
```

Changes in final project:

```text
RTSP URL parsing is configurable through app.conf
SETUP can use SDP a=control
RTP over TCP interleaved frames are parsed continuously
H264/H265 RTP packets are converted to Annex-B NALU packets
```

## camera_event_agent

Original role:

```text
HTTP multipart upload
```

Current implementation in final project:

```text
camera_abnormal_app/src/uploader.c
```

Changes in final project:

```text
No libcurl dependency
Pure socket HTTP multipart/form-data upload
Upload retry
Server 2xx confirmation
```

## New Modules

These modules did not exist as complete modules in the original demos:

```text
camera_abnormal_app/src/ring_buffer.c
camera_abnormal_app/src/clip_writer.c
camera_abnormal_app/src/event_queue.c
camera_abnormal_app/server/server.py
```

Their roles:

```text
ring_buffer.c     timestamped H264/H265 Annex-B memory ring buffer
clip_writer.c     pre/post event clipping, key-frame alignment, SPS/PPS/VPS injection
event_queue.c     thread-safe abnormal event handoff
server.py         external server receiver and MP4 conversion
```

## Build Independence

The final project no longer depends on this external source path during build:

```text
/heop/demo/human_detect_demo_v2
```

The Makefile uses only:

```text
./src
./vendor/human_detect_demo_v2
/heop/include
/heop/lib
```

`/heop/include` and `/heop/lib` are still required because they are the HEOP
SDK and device runtime libraries, not the original demo project.
