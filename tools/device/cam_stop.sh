#!/bin/sh
# Stop the stock camera stack (vif.sh watchdog, gkuui UI, cardv) without rebooting.
# cardv's exit handler blocks on "cat /proc/kmsg | tee ..."; kill that cat to let it finish.
pkill -f /customer/gadget/vif.sh
killall gkuui 2>/dev/null
killall cardv 2>/dev/null
i=0
while pidof cardv >/dev/null && [ $i -lt 30 ]; do
    pkill -f 'cat /proc/kmsg' 2>/dev/null
    sleep 1
    i=$((i + 1))
done
rm -f /mnt/mmc/kmsg_info_*.txt /mnt/mmc/pid*_maps.txt
if pidof cardv >/dev/null; then echo "cam_stop: cardv still running"; exit 1; fi
# cardv unmounts the SD card on exit; bring it back for our recordings.
if ! grep -q " /app/bin/sd " /proc/mounts && [ -b /dev/mmcblk0p1 ]; then
    mount -t vfat /dev/mmcblk0p1 /app/bin/sd && echo "cam_stop: SD remounted"
fi
echo "cam_stop: stopped"
