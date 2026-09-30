#!/usr/bin/env python3
"""Push a small file to the camera over the UART shell (base64 chunks), verify md5.

Usage: uart_push.py <local_file> <remote_path>
The device shell must be idle at its prompt. Only one program may hold the port.
Files are gzipped before sending when that makes them smaller (the camera has gunzip), and sent
in 900-character chunks (busybox's line editor caps a line at 1024), each waiting for the actual transmission
(flush) instead of a fixed sleep: a 178 KB camapp went from ~465 round trips to ~160.
"""
import base64, gzip, hashlib, serial, sys, time

PORT, BAUD, CHUNK = '/dev/ttyUSB0', 115200, 900

def run(s, cmd, wait=0.3):
    s.write(cmd.encode() + b'\n')
    time.sleep(wait)
    return s.read(65536).decode('utf-8', 'replace')

def main():
    local, remote = sys.argv[1], sys.argv[2]
    data = open(local, 'rb').read()
    packed = gzip.compress(data, 9)
    zipped = len(packed) < len(data) * 0.9
    b64 = base64.b64encode(packed if zipped else data).decode()
    tmp = remote + '.b64'
    s = serial.Serial(PORT, BAUD, timeout=0.2)
    s.reset_input_buffer()
    run(s, f'rm -f {tmp} {remote}')
    for i in range(0, len(b64), CHUNK):
        s.write(f"echo -n '{b64[i:i+CHUNK]}' >> {tmp}\n".encode())
        s.flush()                                   # wait until the line is on the wire
        time.sleep(0.05)                            # let the shell run it
        s.reset_input_buffer()                      # drop the echoed line
    decode = f'base64 -d {tmp} | gunzip > {remote}' if zipped else f'base64 -d {tmp} > {remote}'
    run(s, f'{decode} && rm {tmp} && chmod +x {remote}', 1.0)
    out = run(s, f'md5sum {remote}', 0.8)
    want = hashlib.md5(data).hexdigest()
    ok = want in out
    print(f'{local} -> {remote}: {len(data)} bytes, md5 {want} {"OK" if ok else "MISMATCH"}')
    if not ok:
        print(out)
        sys.exit(1)

if __name__ == '__main__':
    main()
