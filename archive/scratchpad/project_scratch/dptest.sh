#!/bin/bash
# dptest.sh TAG "dp args": set DP mode, take a photo, pull it, count isolated bright pixels
cd /home/aftwr/microscopeproject
python3 tools/sh.py "" "echo dp $2 > /tmp/camapp_fifo" >/dev/null; sleep 2
python3 tools/sh.py "" "echo snap > /tmp/camapp_fifo" >/dev/null; sleep 3
F=$(python3 tools/sh.py "" "ls /app/bin/sd/IMG_*.jpg | tail -1" | grep -o '/app/bin/sd/IMG_[0-9]*.jpg' | tail -1)
timeout 200 python3 tools/uart_pull.py $F scratch/dp_$1.jpg | tail -1
python3 - "$1" <<'PY'
import sys, numpy as np
from PIL import Image
from scipy.ndimage import median_filter
a=np.asarray(Image.open(f'scratch/dp_{sys.argv[1]}.jpg').convert('L')).astype(int)
m=median_filter(a,3); d=a-m
print(sys.argv[1], 'mean', round(a.mean(),1), 'spikes>40', int((d>40).sum()), 'spikes>80', int((d>80).sum()))
PY
