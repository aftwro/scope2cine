#!/bin/bash
# viewgrab.sh NAME: the camera's processed picture (160x90) over UART into scratch/NAME.png -
# for checking the image when neither the HDMI monitor nor the USB webcam can be watched.
N=${1:-view}; D=$(dirname "$0")/..; mkdir -p $D/scratch
python3 $D/tools/sh.py "" "echo viewgrab > /tmp/camapp_fifo" > /dev/null; sleep 4
timeout 300 python3 $D/tools/uart_pull.py /tmp/view.ppm $D/scratch/$N.ppm | tail -1
python3 -c "from PIL import Image; Image.open('$D/scratch/$N.ppm').resize((640,360), Image.NEAREST).save('$D/scratch/$N.png')" && echo "saved scratch/$N.png"
