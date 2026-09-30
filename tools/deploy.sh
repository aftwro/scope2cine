#!/bin/sh
# Build camapp, check symbols, push it and install it on the camera with automatic rollback.
# Usage: tools/deploy.sh [binary]    (default: build src/camapp and deploy that)
set -e
cd "$(dirname "$0")/.."
BIN=${1:-src/camapp/camapp}
if [ -z "$1" ]; then (cd src/camapp && ./build.sh); fi
case "$BIN" in *.sh) ;; *) tools/check_syms.sh "$BIN" ;; esac
tools/uart_push.py tools/device/flash/update.sh /tmp/update.sh
tools/uart_push.py tools/device/flash/rollback.sh /customer/camapp/rollback.sh
tools/uart_push.py "$BIN" /tmp/camapp.new
python3 - <<'PY'
import re, serial, sys, time
s = serial.Serial('/dev/ttyUSB0', 115200, timeout=0.3)
s.reset_input_buffer()
s.write(b'/tmp/update.sh /tmp/camapp.new; echo UPDATE_RC=$?\n')
buf, t = b'', time.time()
while time.time() - t < 90 and not re.search(rb'UPDATE_RC=\d+\r?\n', buf):
    buf += s.read(4096)
text = ''.join(c for c in buf.decode('utf-8', 'replace') if c.isprintable() or c == '\n')
for line in text.splitlines():
    if line.startswith(('update:', 'UPDATE_RC')) or 'camapp' in line and 'broken' in line:
        print(line)
m = re.search(r'UPDATE_RC=(\d+)', text)
sys.exit(int(m.group(1)) if m else 3)
PY
