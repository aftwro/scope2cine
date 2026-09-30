#!/bin/sh
# Start the stock camera stack the way /bootconfig/demo.sh does.
# Optional: CAM_PRELOAD=/path/lib.so to inject a shim into cardv.
INI=/customer/cardvini/one_bayer_4k.ini
grep -q "FB_WIDTH = 1920" /misc/config.ini && INI=/customer/cardvini/one_bayer_1080.ini
cd /
if [ -n "$CAM_PRELOAD" ]; then
    LD_PRELOAD=$CAM_PRELOAD cardv $INI &
else
    cardv $INI &
fi
sleep 15
# vif.sh pokes the sensor over I2C when frames stop; never run it without cardv.
if ! pidof cardv >/dev/null; then
    echo "cam_start: cardv failed to start, vif.sh not started"
    exit 1
fi
riu_w 0x1154 16 0x0613
/customer/gadget/vif.sh > /dev/null 2>&1 &
echo "cam_start: started ($INI${CAM_PRELOAD:+, preload $CAM_PRELOAD})"
