import serial, re, sys, time
s = serial.Serial('/dev/ttyUSB0', 115200, timeout=0.2)
log = open('/home/aftwr/microscopeproject/logs/usb_plug_2026-09-24.log', 'ab')
pat = re.compile(r'USB|GADGET|uvc|UVC|g_webcam|video0|ERR|Segmentation|U-Boot|Starting kernel|reboot|Oops|panic', re.I)
buf = b''
while True:
    d = s.read(4096)
    if not d: continue
    log.write(d); log.flush()
    buf += d
    *lines, buf = buf.split(b'\n')
    for l in lines:
        t = l.decode('utf-8', 'replace').replace('\r', '').strip()
        if t and pat.search(t):
            print('UART: ' + t[:200], flush=True)
