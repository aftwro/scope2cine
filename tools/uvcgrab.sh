#!/bin/bash
# uvcgrab.sh NAME: grab one 1080p frame from the camera's USB webcam via Windows ffmpeg (dshow)
# into scratch/NAME.jpg (keeps the 5th frame: the first one after a reconnect can be empty).
N=${1:-uvc}; T=/mnt/c/Users/aftwr/AppData/Local/Temp
( cd $T && timeout 40 /mnt/c/ProgramData/chocolatey/bin/ffmpeg.exe -y -hide_banner -loglevel error -f dshow \
    -vcodec mjpeg -video_size 1920x1080 -i video="USB Video Device" -frames:v 5 -update 1 -c copy $N.jpg )
mkdir -p "$(dirname "$0")/../scratch" && cp $T/$N.jpg "$(dirname "$0")/../scratch/$N.jpg" && echo "saved scratch/$N.jpg"
