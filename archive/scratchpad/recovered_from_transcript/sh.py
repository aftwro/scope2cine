import serial, sys, time
s = serial.Serial('/dev/ttyUSB0', 115200, timeout=0.2)
s.reset_input_buffer()
for cmd in sys.argv[1:]:
    s.write(cmd.encode() + b'\n')
    buf = bytearray(); t0 = time.time(); last = time.time()
    while time.time() - t0 < 15:
        d = s.read(4096)
        if d: buf += d; last = time.time()
        elif time.time() - last > 1.0 and buf: break
    print(buf.decode('utf-8','replace').replace('\r',''))
