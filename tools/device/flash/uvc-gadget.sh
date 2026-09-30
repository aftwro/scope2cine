#!/bin/sh
# Installed as /customer/camapp/uvc-gadget.sh; boot.sh runs it before camapp.
# Replaces the vendor g_webcam module (fake 320x240..3840x2160 list plus a still-image pin
# that the Windows Camera app waits on and times out) with our own configfs UVC gadget:
# one MJPEG format at 1920x1080, the size camapp records, at 30 or 60 fps, no still image.
# The vendor udc-uvc.sh already loaded libcomposite + usb_f_uvc and mounted configfs.
# On any failure the vendor g_webcam is loaded again, so the camera keeps a webcam.
G=/customer/config/usb_gadget/cam
F=$G/functions/uvc.usb0
UDC=$(ls /sys/class/udc | head -1)

fallback() {
    echo "uvc-gadget: $1, falling back to g_webcam"
    [ -f $G/UDC ] && echo "" > $G/UDC 2>/dev/null
    lsmod | grep -q '^g_webcam' || insmod /customer/modules/4.9.227/g_webcam.ko streaming_maxpacket=3072 uac_function_enable=0
    exit 1
}

if lsmod | grep -q '^g_webcam'; then
    rmmod g_webcam || fallback "rmmod g_webcam failed"
fi

if [ ! -d $G ]; then
    mkdir $G || fallback "no configfs"
    echo 0x0AC8 > $G/idVendor
    echo 0x3420 > $G/idProduct
    echo 0x0500 > $G/bcdDevice          # new device revision: hosts re-read the descriptors
    echo 0xEF > $G/bDeviceClass         # miscellaneous / IAD, as UVC requires
    echo 0x02 > $G/bDeviceSubClass
    echo 0x01 > $G/bDeviceProtocol
    mkdir $G/strings/0x409
    echo "camapp" > $G/strings/0x409/manufacturer
    echo "camapp IMX307" > $G/strings/0x409/product
    echo "0001" > $G/strings/0x409/serialnumber

    mkdir -p $F/control/header/h || fallback "no uvc function"
    ln -s $F/control/header/h $F/control/class/fs
    ln -s $F/control/header/h $F/control/class/ss

    M=$F/streaming/mjpeg/m/1080p
    mkdir -p $M
    echo 1920 > $M/wWidth
    echo 1080 > $M/wHeight
    echo 83886080 > $M/dwMinBitRate      # ~350 KB frames at 30 fps
    echo 167772160 > $M/dwMaxBitRate     # ... at 60 fps
    echo 2097152 > $M/dwMaxVideoFrameBufferSize
    echo 333333 > $M/dwDefaultFrameInterval
    printf "333333\n166666\n" > $M/dwFrameInterval
    mkdir $F/streaming/header/h
    ln -s $F/streaming/mjpeg/m $F/streaming/header/h/m
    ln -s $F/streaming/header/h $F/streaming/class/fs
    ln -s $F/streaming/header/h $F/streaming/class/hs
    echo 3072 > $F/streaming_maxpacket   # 3 x 1024 bytes per microframe, as g_webcam used

    mkdir -p $G/configs/c.1/strings/0x409
    echo "Video" > $G/configs/c.1/strings/0x409/configuration
    echo 100 > $G/configs/c.1/MaxPower
    ln -s $F $G/configs/c.1/
fi

echo "$UDC" > $G/UDC || fallback "bind to $UDC failed"
echo "uvc-gadget: bound to $UDC"
