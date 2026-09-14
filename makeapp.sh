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
pack.sh
execute_result_proc "execute pack.sh"
cd - >/dev/null || exit 1

mkdir -p "$BASE_PATH/output"
mv "$APP_PATH"/cameraAbnormal_*.app "$BASE_PATH/output"/
execute_result_proc "move app package"
