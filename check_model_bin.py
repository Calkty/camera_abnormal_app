#!/usr/bin/env python3
"""Check a HIKFlow model BIN before it is flashed to the device.

The device derives its input configuration from a 256 byte info block appended
to the model BIN. `hikflow_proc_update_model_info()` in
`vendor/human_detect_demo_v2/src/hikflow/code/hikflow_proc_priv.c` reads it, and
`hikflow_proc_update_net_info()` turns `dformat` into the source pixel format it
declares to `opdevsdk_hikflow_Process`.

That declaration must match the buffer the runtime actually passes. The runtime
hands over an NV21 (YUV420SP) frame produced by mscale, so **dformat must be 2**.
A BIN that declares BGR (dformat 1) makes the SDK interpret a ~614 KB NV21 frame
as a 1.2 MB packed BGR image: the model then sees garbage, the custom layer
emits no boxes, and nothing is logged as an error. That is a silent
"no detections" failure, which is why this check exists.

Usage:
    python check_model_bin.py <bin> [<bin> ...]
    python check_model_bin.py git:HEAD:vendor/.../bin/Model_P_NPU0.bin

Read-only. Run it before every `make install`/`makeapp.sh` after a model swap.
"""

import os
import struct
import subprocess
import sys

INFOLEN = 256  # HIKFLOW_DEMO_MODEL_INFOLEN

# value -> (name, what the runtime must supply)
DFORMAT_NAMES = {
    1: "BGR (packed 3 channel)",
    2: "YVU420 / NV21",
    4: "YUV420 / NV12",
}
EXPECTED_DFORMAT = 2  # mscale always produces NV21; see hikflow_proc_net()
EXPECTED_DTYPE = 1    # hikflow_proc_net() hardcodes OPDEVSDK_HKA_DATA_U08

# tail offsets, verified byte by byte against a real BIN
OFF = {"dev_info": 9, "plat_type": 23, "dtype": 33, "dformat": 45, "innum": 55}


def load(path):
    if path.startswith("git:"):
        return subprocess.run(["git", "show", path[4:]], capture_output=True).stdout
    with open(path, "rb") as f:
        return f.read()


def decode(buf):
    """Return the info block as a dict, or None when the BIN carries none."""
    if len(buf) < INFOLEN:
        return None
    tail = buf[-INFOLEN:]
    if not tail.startswith(b"dev_info:"):
        return None

    def i32(off):
        return struct.unpack_from("<i", tail, off)[0]

    info = {k: i32(v) for k, v in OFF.items()}
    info["blobs"] = []
    for i in range(info["innum"]):
        base = 59 + i * 16
        info["blobs"].append(struct.unpack_from("<4i", tail, base))
    return info


def describe(path):
    buf = load(path)
    print("=" * 72)
    print(f"{path}")
    print(f"  size      {len(buf)} bytes")
    if not buf:
        print("  ERROR: empty (did `git show` resolve the path?)")
        return None

    info = decode(buf)
    if info is None:
        print("  ERROR: no trailing 'dev_info:' block in the last 256 bytes.")
        print("         The device would abort with \"doesn't carry information\".")
        print("         Check that this is a HIKFlow BIN produced by tctool.")
        return None

    print(f"  dev_info  {info['dev_info']}")
    print(f"  plat_type {info['plat_type']}")
    print(f"  dtype     {info['dtype']}  (expect {EXPECTED_DTYPE} = U08)")
    name = DFORMAT_NAMES.get(info["dformat"], "unknown")
    print(f"  dformat   {info['dformat']}  ({name})")
    print(f"  innum     {info['innum']}")
    for i, (b, c, h, w) in enumerate(info["blobs"]):
        print(f"  blob[{i}]   batch={b} channel={c} height={h} width={w}")
    return info


def verdict(path, info):
    if info is None:
        return False
    ok = True
    if info["dformat"] != EXPECTED_DFORMAT:
        ok = False
        print(f"  FAIL: dformat={info['dformat']} but the runtime feeds NV21 "
              f"({EXPECTED_DFORMAT}).")
        print("        The model will receive mis-interpreted pixels and detect")
        print("        nothing, with no error in the log.")
        print("        Fix by re-converting so the preprocessing emits NV21 input")
        print("        data (see MODEL_SWAP.md), not by editing the BIN.")
    if info["dtype"] != EXPECTED_DTYPE:
        ok = False
        print(f"  FAIL: dtype={info['dtype']} but hikflow_proc_net() hardcodes U08 "
              f"({EXPECTED_DTYPE}).")
    if not info["blobs"] or info["blobs"][0][1] != 3:
        ok = False
        print("  FAIL: first input blob is not 3 channel.")
    if ok:
        print(f"  OK: dformat={EXPECTED_DFORMAT} (NV21), dtype=U08 -> matches the runtime")
    return ok


def main(argv):
    if not argv:
        print(__doc__)
        return 2

    infos = [(p, verdict(p, info)) for p, info in ((p, describe(p)) for p in argv)]

    if len(argv) >= 2:
        print("=" * 72)
        print("differences that matter (first entry is the reference):")
        ref = decode(load(argv[0]))
        for path in argv[1:]:
            cur = decode(load(path))
            if ref is None or cur is None:
                print(f"  {path}: cannot compare (missing info block)")
                continue
            for key in ("plat_type", "dtype", "dformat", "innum", "blobs"):
                if ref[key] != cur[key]:
                    print(f"  {key}: {ref[key]} -> {cur[key]}   <-- {path}")
                else:
                    print(f"  {key}: unchanged ({ref[key]})")

    print("=" * 72)
    failed = [p for p, ok in infos if not ok]
    if failed:
        print(f"FAIL: {len(failed)} of {len(infos)} BIN(s) would not work on the device:")
        for p in failed:
            print(f"  {p}")
        return 1
    print(f"PASS: all {len(infos)} BIN(s) match the runtime input configuration")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
