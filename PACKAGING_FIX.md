# HEOP startup fix (2026-09-11)

The device log reports that the loader cannot find libusr_trans.so.
Both projects link with -lusr_trans. It is not a new dependency.

Changes:
- Include the application directory in LD_LIBRARY_PATH.
- Install libusr_trans.so and its versioned files from the build SDK.
- Recreate the demo's /heop/package layout under cameraAbnormal.
- Preserve user_data and use the cameraAbnormal config/audio paths.
- Register the SDK and protocol routes under cameraAbnormal.
- Fail packaging early when required application resources are missing.

In the H9 HEOP build environment, from this project directory:

```sh
sh makeapp.sh CC=aarch64-mix210-linux-gcc ENABLE_INFER=1 ENABLE_RING=0 ENABLE_CLIP=0 ENABLE_UPLOAD=0 ENABLE_LEGACY_ALARM=0
```

The generated installer is output/cameraAbnormal_*.app. The source ZIP
is not an installer. Build and device validation have not been completed
in this revision because access to the existing Docker execution API was
denied. Other SDK shared libraries are still provided by the HEOP base
package, as in the original demo; their device compatibility is unverified.

The package keeps appID/aid 19999 consistent with the vendored source.
Concurrent installation with humanDetect using the same ID is unverified.
The bundled model is the original Model_P_NPU0.bin. app.conf currently
selects abnormal_classes=1,2,3; person is class 0 in that model. Adjust the
class selection for the intended experiment before building.

On the camera, verify that cameraAbnormal_1.log passes the startup message,
then inspect inference initialization and detection output. Log file count
alone is not a successful-inference test.
