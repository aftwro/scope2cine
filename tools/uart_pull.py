#!/usr/bin/env python3
"""Pull a file from the camera over the UART shell, verify md5.

Usage: uart_pull.py <remote_path> <local_file>

First the whole file is streamed at once (gzip | base64 between markers) and checked against the
file's md5 - seconds instead of minutes (a 43 KB file took ~4 min in 1 KB round trips). The console
is shared with kernel output, which can interleave with the transfer; if the stream fails its
check, the file is moved in 1 KB chunks, each base64-encoded on one line between markers and
checked against its own md5, bad chunks retried. The shell must be idle at its prompt; only one
program may hold the port.
"""
import base64, gzip, hashlib, re, serial, sys, time

PORT, BAUD, CHUNK, RETRIES = '/dev/ttyUSB0', 115200, 1024, 5


def ask(s, cmd, end_marker, timeout=10):
    s.reset_input_buffer()
    s.write(cmd.encode() + b'\n')
    buf, t = b'', time.time()
    while time.time() - t < timeout:
        buf += s.read(8192)
        if buf.count(end_marker) >= 2:      # once in the echoed command, once in the output
            break
    return buf.decode('utf-8', 'replace').replace('\r', '')


def main():
    remote, local = sys.argv[1], sys.argv[2]
    s = serial.Serial(PORT, BAUD, timeout=0.2)

    out = ask(s, f'echo @@S $(wc -c < {remote}) $(md5sum {remote}) @@E', b'@@E')
    m = re.search(r'@@S (\d+) ([0-9a-f]{32}) ', out)
    if not m:
        sys.exit(f'cannot stat {remote}:\n{out}')
    size, want = int(m.group(1)), m.group(2)

    s.reset_input_buffer()
    s.write(f'echo @@B; gzip -c {remote} | base64; echo @@Z\n'.encode())
    buf, t = b'', time.time()
    while time.time() - t < 30 + size / 2000 and buf.count(b'@@Z') < 2:
        buf += s.read(65536)
    m = re.search(rb'\n@@B\r?\n(.*?)@@Z', buf, re.S)
    if m:
        try:
            data = gzip.decompress(base64.b64decode(re.sub(rb'[^A-Za-z0-9+/=]', b'', m.group(1))))
        except Exception:
            data = b''
        if hashlib.md5(data).hexdigest() == want and len(data) == size:
            open(local, 'wb').write(data)
            print(f'{remote} -> {local}: {len(data)} bytes, md5 {want} OK (stream)')
            return
    print('stream check failed, falling back to 1 KB chunks')

    data = bytearray()
    for i in range((size + CHUNK - 1) // CHUNK):
        cmd = (f'dd if={remote} bs={CHUNK} skip={i} count=1 2>/dev/null > /tmp/.chunk; '
               f'echo @@C$(md5sum < /tmp/.chunk | cut -c1-32):$(base64 /tmp/.chunk | tr -d "\\n"):@@E')
        for attempt in range(RETRIES):
            out = ask(s, cmd, b':@@E')
            got = None
            for c in re.finditer(r'@@C([0-9a-f]{32}):([A-Za-z0-9+/=]*):@@E', out):
                try:
                    blob = base64.b64decode(c.group(2), validate=True)
                except ValueError:
                    continue
                if hashlib.md5(blob).hexdigest() == c.group(1):
                    got = blob
            if got is not None:
                data += got
                break
        else:
            sys.exit(f'chunk {i} failed after {RETRIES} attempts')

    open(local, 'wb').write(data)
    got = hashlib.md5(data).hexdigest()
    ok = got == want and len(data) == size
    print(f'{remote} -> {local}: {len(data)} bytes, md5 {got} {"OK" if ok else "MISMATCH"}')
    sys.exit(0 if ok else 1)


if __name__ == '__main__':
    main()
