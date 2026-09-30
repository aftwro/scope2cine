import serial, sys, time
cmd = sys.argv[1]; dur = float(sys.argv[2]); out = sys.argv[3]
s = serial.Serial('/dev/ttyUSB0', 115200, timeout=0.2)
s.reset_input_buffer()
s.write(b'\n'); time.sleep(0.5)
pre = s.read(4096)
s.write(cmd.encode() + b'\n')
buf = bytearray(pre); t0 = time.time()
while time.time() - t0 < dur:
    buf += s.read(4096)
open(out, 'wb').write(buf)
print(buf.decode('utf-8', 'replace'))
