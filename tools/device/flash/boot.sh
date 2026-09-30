#!/bin/sh
# Installed as /customer/camapp/boot.sh; started by /config/cardv instead of stock cardv
# when /customer/camapp/enable exists. The wrapper counts boot attempts; this script
# resets the counter once camapp has run for 30 s, else 3 bad boots fall back to stock.
D=/customer/camapp

touch /tmp/cardv_rec                        # demo.sh's wait_rec.sh waits for this marker

# demo.sh starts the vif.sh sensor watchdog right after us; it would poke the sensor over I2C.
( i=0; while [ $i -lt 60 ]; do pkill -f /customer/gadget/vif.sh && break; sleep 1; i=$((i + 1)); done ) &

# cardv used to mount the SD card; do it ourselves. The card is still being detected
# this early in boot, so retry for up to 20 s in the background.
( i=0; while [ $i -lt 20 ]; do
      grep -q " /app/bin/sd " /proc/mounts && break
      [ -b /dev/mmcblk0p1 ] && mount -t vfat /dev/mmcblk0p1 /app/bin/sd 2>/dev/null && echo "camapp: SD mounted" && break
      sleep 1; i=$((i + 1))
  done ) &

# Our own UVC gadget (honest 1920x1080 MJPEG, 30/60 fps, no still-image pin) replaces the
# vendor g_webcam that demo.sh loaded; the script falls back to g_webcam by itself.
[ -x $D/uvc-gadget.sh ] && $D/uvc-gadget.sh

$D/camapp > /tmp/camapp.log 2>&1 &
pid=$!
echo "camapp: started pid $pid"
sleep 30
if kill -0 $pid 2>/dev/null; then
    echo 0 > $D/boot_attempts
    sync
    echo "camapp: healthy after 30 s, boot counter reset"
else
    echo "camapp: exited early, see /tmp/camapp.log"
fi
