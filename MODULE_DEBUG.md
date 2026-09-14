# Module Debug Builds

Compile-time switches live in `src/module_flags.h`. All default to 1 to
preserve the previous behavior. Set a macro to 0 in that header, or override
it with the Make variables below. No app.conf key can enable a compiled-out
feature. Keep the same Make flags when running `make install`.

| Make variable | C macro | Controls |
| --- | --- | --- |
| ENABLE_INFER | CA_ENABLE_INFER | BSC/VIN, mscale and HIKFlow startup |
| ENABLE_RING | CA_ENABLE_RING | RTSP recording thread and compressed packet ring |
| ENABLE_CLIP | CA_ENABLE_CLIP | Event queue and local Annex-B clip generation |
| ENABLE_UPLOAD | CA_ENABLE_UPLOAD | Upload queue, HTTP upload, retry and acknowledgement |
| ENABLE_LEGACY_ALARM | CA_ENABLE_LEGACY_ALARM | Legacy alarm threads, audio initialization and post-detection JPEG/HEOP alarm linkage |

CLIP requires RING. UPLOAD requires CLIP. Invalid combinations fail at compile
time. LEGACY_ALARM has no effect when INFER is disabled. Basic POS target/text
submission remains part of inference, independent of legacy alarm linkage.
Platform registration/configuration remains enabled with inference; disabling
legacy alarms does not disable all HEOP communication or guarantee no app.log
errors. Successful POS display still depends on firmware, stream and player.

## Build Examples

Run inside the existing HEOP cross-compilation container at the project root.

Inference only (raw VIN frames, not RTSP decoding):

```sh
make CC=aarch64-mix210-linux-gcc ENABLE_INFER=1 ENABLE_RING=0 ENABLE_CLIP=0 ENABLE_UPLOAD=0 ENABLE_LEGACY_ALARM=0
```

RTSP and ring only (no model initialization):

```sh
make CC=aarch64-mix210-linux-gcc ENABLE_INFER=0 ENABLE_RING=1 ENABLE_CLIP=0 ENABLE_UPLOAD=0 ENABLE_LEGACY_ALARM=0
```

RTSP, ring and local clips with synthetic events:

```sh
make CC=aarch64-mix210-linux-gcc ENABLE_INFER=0 ENABLE_RING=1 ENABLE_CLIP=1 ENABLE_UPLOAD=0 ENABLE_LEGACY_ALARM=0
```

Set `test_event_interval_seconds=30` and `debug_level=1` in app.conf. Synthetic
events use the infer supervisor thread but do not initialize VIN or HIKFlow.
The first test event occurs immediately, so its pre-event history can be short.
Wait for subsequent events before checking pre/post duration. With UPLOAD=0,
video and JSON remain in `<work_dir>/events`; no upload queue is allocated.

To test HTTP upload without a model, use the preceding command with
`ENABLE_UPLOAD=1`, and configure a reachable `upload_url` in app.conf. Upload
still requires a generated clip; a standalone existing-file upload mode is
not provided by these switches.

Full pipeline without legacy demo alarms:

```sh
make CC=aarch64-mix210-linux-gcc ENABLE_INFER=1 ENABLE_RING=1 ENABLE_CLIP=1 ENABLE_UPLOAD=1 ENABLE_LEGACY_ALARM=0
```

Set `test_event_interval_seconds=0` for real model events. To debug detection
and local clips only, change ENABLE_UPLOAD to 0. Configure model path, classes,
app_chan and inference interval in app.conf as before.

## Deployment and Verification

Use the same flags with `make install`; otherwise Make returns to header
defaults and rebuilds. Changes to the header or Make module flags trigger an
automatic rebuild of all objects, including vendored objects. A clean build
is also supported with `make clean` before the desired command.

Start from the APP directory using the existing HEOP launch process. The startup
log prints `modules: infer=... ring_rtsp=... clip=... upload=... legacy_alarm=...`.
Core logs use stderr (the application/container log); vendor HIKFlow logs use
the existing DSP logger. Set debug_level=1 for bridge and clip diagnostics,
or 2 for detailed queue/packet diagnostics.

When CLIP=0, model alarms are logged but not enqueued. When UPLOAD=0, local
clips are retained and no queue can fill waiting for an absent upload worker.
Local clip storage is not automatically bounded: monitor disk space during
long debugging sessions. Module changes require recompilation and restart.

These switches isolate module execution and resource allocation. The Makefile
still builds and links vendor sources, so the HEOP SDK/toolchain is still
required even for RTSP-only builds. They do not provide runtime restart of
vendor threads; restart the process when changing the enabled combination.
