#!/bin/sh

APP_DIR="$(cd "$(dirname "$0")" && pwd)"
RUNTIME_DIR=/heop/package/cameraAbnormal
mkdir -p "$RUNTIME_DIR/user_data" || exit 1
# Match the HEOP demo's layout while retaining persistent user data.
for item in "$APP_DIR"/*; do
    [ -e "$item" ] || continue
    name=${item##*/}
    [ "$name" = user_data ] && continue
    if [ "$APP_DIR" != "$RUNTIME_DIR" ]; then
        ln -sfn "$item" "$RUNTIME_DIR/$name" || exit 1
    fi
done
cd "$RUNTIME_DIR" || exit 1

export LD_LIBRARY_PATH="$APP_DIR:$RUNTIME_DIR:/heop/lib:/heop/lib/bll:/heop/lib/scheduler:/heop/lib/hikflow:/heop/lib/bsc:/heop/dsp/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"

echo "cameraAbnormal: starting from $RUNTIME_DIR"
exec "$RUNTIME_DIR/camera_abnormal_app" "$RUNTIME_DIR/app.conf"
