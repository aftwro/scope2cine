#!/bin/sh
# Re-push our device-side tools to /tmp (RAM) after a camera reboot.
set -e
cd "$(dirname "$0")/.."
for f in src/camapp/camapp:/tmp/camapp \
         src/mitrace/mitrace.so:/tmp/mitrace.so \
         tools/device/cam_stop.sh:/tmp/cam_stop.sh \
         tools/device/cam_start.sh:/tmp/cam_start.sh; do
    tools/uart_push.py "${f%%:*}" "${f#*:}"
done
