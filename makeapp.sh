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
