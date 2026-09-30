#!/bin/bash
# usage: hset.sh HMAX - write HMAX in standby and leave it (undo with regundo + framerate)
H=$1; L=$((H & 255)); M=$((H >> 8))
python3 sh.py "" "F=/tmp/camapp_fifo; echo regw 0x3000 1 > \$F; sleep 0.1; echo regw 0x301C $L > \$F; echo regw 0x301D $M > \$F; sleep 0.1; echo regw 0x3000 0 > \$F; sleep 3; echo sensor > \$F" > /dev/null
sleep 5
python3 sh.py "" "grep -E 'measured|regs VMAX' /tmp/camapp.log | tail -2" | grep sensor
