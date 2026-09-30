#!/bin/bash
# usage: hsweep.sh HMAX_HEX NAME
H=$1; N=$2; L=$((H & 255)); M=$((H >> 8))
C="F=/tmp/camapp_fifo; echo regw 0x3000 1 > \$F; sleep 0.1; echo regw 0x301C $L > \$F; echo regw 0x301D $M > \$F; sleep 0.1; echo regw 0x3000 0 > \$F; sleep 3; echo sensor > \$F; sleep 12; echo regw 0x3000 1 > \$F; sleep 0.1; echo regundo > \$F; sleep 1; echo framerate 60 > \$F; sleep 3; echo sensor > \$F; sleep 4"
(sleep 8; timeout 60 ./brio.sh $N > /dev/null 2>&1) &
timeout 90 python3 sh.py "" "$C" > /dev/null 2>&1
wait
sleep 4
timeout 30 python3 sh.py "" "grep -E 'measured' /tmp/camapp.log | tail -2; head -17 /sys/class/mstar/csi0/csi_ints | grep -v ': 0' | grep ERR; pidof camapp" 2>&1 | grep -E "measured|ERR|^[0-9]+$" | tail -4
