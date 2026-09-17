#!/bin/sh
set -eu

execute_result_proc()
{
    if [ $? -eq 0 ]; then
        echo "$* succeed!!!"
    else
        echo "$* failed!!!"
        exit 1
    fi
}

BASE_PATH=$(cd "$(dirname "$0")" && pwd)
APP_PATH=$BASE_PATH/APP

# Every HIKFlow model BIN ends with a 256-byte dev_info block. The input format
# the model declares lives at byte offset 45 inside it (see check_model_bin.py).
# This app always feeds the NPU NV21 frames, so the model must declare 2.
# A mismatch is silent: the model loads, the NPU runs, and every frame yields
# zero boxes with nothing in the log (see MODEL_SWAP.md).
model_dformat()
{
    bin=$1
    size=$(wc -c < "$bin" | tr -d ' ')
    dd if="$bin" bs=1 skip=$((size - 256 + 45)) count=4 2>/dev/null | od -An -tu4 | tr -d ' \n'
}

make -C "$BASE_PATH" clean
make -C "$BASE_PATH" "$@"
execute_result_proc "execute Makefile"

make -C "$BASE_PATH" install "$@"
execute_result_proc "execute Makefile install"

cd "$APP_PATH" || exit 1
test -s camera_abnormal_app
test -s libusr_trans.so
test -s Model_P_NPU0.bin
test -s hikflow_config.json
test -s hikflow_attr.json
sh -n cameraAbnormal.sh

dformat=$(model_dformat Model_P_NPU0.bin)
case "$dformat" in
    2) echo "model input format: 2 (NV21) ok" ;;
    1|4)
        echo "ERROR: Model_P_NPU0.bin declares input format $dformat, but this app" >&2
        echo "       always feeds NV21 (2). The model would load, run, and detect" >&2
        echo "       nothing, with no error in the log." >&2
        echo "       Re-convert with \"input_type\": 2 and copy bin + out/tctool" >&2
        echo "       into vendor/human_detect_demo_v2/src/hikflow/. See MODEL_SWAP.md." >&2
        exit 1
        ;;
    *)
        echo "ERROR: cannot read the model input format from Model_P_NPU0.bin" >&2
        echo "       (read '$dformat' at dev_info offset 45; expected 2)." >&2
        echo "       Is this really a HIKFlow model BIN? See MODEL_SWAP.md." >&2
        exit 1
        ;;
esac

# A package left over from an earlier build would be swept into output/ by the
# move below and would also be packed into the new package, so start clean.
rm -f cameraAbnormal_*.app

pack.sh
execute_result_proc "execute pack.sh"

# pack.sh names the package from META-INFO as <App-Name>_<version>_<platform>.app
package=
count=0
for f in cameraAbnormal_*.app; do
    [ -e "$f" ] || continue
    package=$f
    count=$((count + 1))
done
if [ "$count" -ne 1 ]; then
    echo "expected exactly one .app from pack.sh, got $count: $package" >&2
    exit 1
fi
cd - >/dev/null || exit 1

mkdir -p "$BASE_PATH/output"
for old in "$BASE_PATH"/output/cameraAbnormal_*.app; do
    [ -e "$old" ] || continue
    echo "remove previous package $(basename "$old")"
    rm -f "$old"
done
mv "$APP_PATH/$package" "$BASE_PATH/output"/
execute_result_proc "move app package"
echo "installer: output/$package"
