#!/bin/sh
# Grab one frame of the monitor from the BRIO webcam (Windows side, dshow) into the scratchpad.
# Usage: brio.sh [name]. dshow is slow to start/stop: the frame is written well before ffmpeg exits.
N=${1:-brio}
W="C:\\Users\\aftwr\\AppData\\Local\\Temp\\$N.jpg"
L="/mnt/c/Users/aftwr/AppData/Local/Temp/$N.jpg"
rm -f "$L"
timeout 45 /mnt/c/ProgramData/chocolatey/bin/ffmpeg.exe -hide_banner -loglevel error -f dshow -rtbufsize 64M \
  -video_size 1920x1080 -i video="BRIO 4K Stream Edition" -vf "select=gte(n\,10)" -frames:v 1 -y "$W" </dev/null >/dev/null 2>&1 &
i=0; while [ ! -s "$L" ] && [ $i -lt 45 ]; do sleep 1; i=$((i+1)); done
sleep 1
cp "$L" "/tmp/claude-1000/-home-aftwr-microscopeproject/9d550714-17fb-41e2-87ba-ede55841232b/scratchpad/$N.jpg" 2>/dev/null && echo "saved $N.jpg after ${i}s" || echo "no frame"
