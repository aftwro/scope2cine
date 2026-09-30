import serial, subprocess, time, re
s = serial.Serial('/dev/ttyUSB0', 115200, timeout=0.2)
log = open('/home/aftwr/microscopeproject/logs/iso_test_uart.log', 'ab')
def uart(cmd, wait=1.5):
    s.reset_input_buffer(); s.write(cmd.encode()+b'\n'); b=b''; t=time.time()
    while time.time()-t < wait: b += s.read(4096)
    log.write(b); return b.decode('utf-8','replace').replace('\r','')
def grab(name):
    subprocess.run(f"timeout 20 ffmpeg -hide_banner -loglevel quiet -f v4l2 -input_format mjpeg -video_size 1920x1080 -framerate 30 -i /dev/video0 -frames:v 30 -c copy -y {name}.mjpeg", shell=True)
    subprocess.run(f"ffmpeg -hide_banner -loglevel quiet -i {name}.mjpeg -vf 'select=gte(n\\,28)' -frames:v 1 -update 1 -y {name}.jpg", shell=True)
    o = subprocess.run(f"ffmpeg -hide_banner -loglevel quiet -i {name}.jpg -vf signalstats,metadata=print:key=lavfi.signalstats.YAVG:file=- -f null -", shell=True, capture_output=True, text=True).stdout
    m = re.search(r'YAVG=([\d.]+)', o); return float(m.group(1)) if m else -1
for v in [0, 1, 2, 3, 4, 5, 0]:
    out = uart(f'echo iso {v} > /tmp/cardv_fifo', 2.0)
    y = grab(f'iso_{v}')
    lines = [l for l in out.splitlines() if re.search(r'ISO|Expo|AE|iso|shutter|Gain', l)]
    print(f'iso {v}: YAVG={y:.1f}  uart: {" | ".join(lines)[:300]}', flush=True)
