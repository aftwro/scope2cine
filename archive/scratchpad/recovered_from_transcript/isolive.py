import serial, subprocess, time, re
s = serial.Serial('/dev/ttyUSB0', 115200, timeout=0.2)
ff = subprocess.Popen("ffmpeg -hide_banner -loglevel quiet -f v4l2 -input_format mjpeg -video_size 1920x1080 -framerate 30 -i /dev/video0 -t 32 -c copy -y isolive.mkv", shell=True)
t0 = time.time(); marks = []
time.sleep(4)
for v in [1, 2, 3, 4, 5, 0]:
    marks.append((v, round(time.time()-t0, 1)))
    s.write(f'echo iso {v} > /tmp/cardv_fifo\n'.encode()); time.sleep(4)
ff.wait()
print('marks (iso, t_sec):', marks)
o = subprocess.run("ffmpeg -hide_banner -loglevel quiet -i isolive.mkv -vf fps=2,signalstats,metadata=print:key=lavfi.signalstats.YAVG:file=- -f null -", shell=True, capture_output=True, text=True).stdout
ys = re.findall(r'YAVG=([\d.]+)', o)
print('YAVG @2fps:', ' '.join(f'{float(y):.0f}' for y in ys))
