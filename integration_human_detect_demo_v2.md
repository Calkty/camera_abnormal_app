# human_detect_demo_v2 Integration Note

This project now vendors the detection-related source code from the original
`human_detect_demo_v2` project under:

```text
vendor/human_detect_demo_v2/
```

The original demo's `src/main.c` is intentionally not included in the build,
because `camera_abnormal_app/src/main.c` is the only application entry point.

## Runtime Flow

The generated application starts the original detection stack from:

```text
camera_abnormal_app/src/infer_adapter.c
```

`infer_thread()` calls the same initialization sequence used by the original
demo:

```text
audio_play_init()
alarm_server_task()
alarm_process()
target_detect_opdevsdk_init()
hikflow_demo_init()
multi_pdc_config_init()
```

`hikflow_demo_init()` then starts the original BSC/VIN, mscale and HIKFlow
capture/inference pipeline.

## Event Hook

The event hook is inside the vendored copy of the original file:

```text
vendor/human_detect_demo_v2/src/hikflow/code/hikflow_demo.c
function: hikflow_demo_proc_alarm(...)
condition: if (1 == alarm_flag)
```

At that point, the code calls:

```c
camera_abnormal_on_human_alarm((int64_t)frame->timeStamp / 1000, 1.0f);
```

The callback is declared in:

```text
camera_abnormal_app/src/event_bridge.h
```

and implemented in:

```text
camera_abnormal_app/src/infer_adapter.c
```

It publishes the unified event:

```c
abnormal_event_publish(g_human_event_queue, "human_abnormal", confidence, event_wall_ms);
```

`infer_adapter.c` checks the timestamp before publishing. If the original frame
timestamp does not look like Unix milliseconds, it falls back to `ca_now_ms()`
so the event time stays aligned with the RTSP ring buffer timestamps.

The rest of the system then clips the RTSP ring buffer and uploads the event
video.

## Why The Original main.c Is Not Built

The original `human_detect_demo_v2/src/main.c` also creates threads and then
loops forever. Building it together with this project would create duplicate
`main()` symbols and conflicting lifecycle control.

So the final application uses:

```text
camera_abnormal_app/src/main.c
```

as the single process controller, and uses `infer_adapter.c` to start the
original detection modules.
