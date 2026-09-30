#!/bin/sh
# AE speed variants: set converge speeds, step -1 EV, trace 1.5 s, back to 0 EV, trace 1.5 s.
F=/tmp/camapp_fifo
v() {
    echo "aeconv 6 $1" > $F; echo "aeconv 7 $2" > $F; echo "aeconv 8 $3" > $F; echo "aeconv 9 $4" > $F
    echo "aevar $1 $2 $3 $4" >> /tmp/aeexp.log
    sleep 2
    echo ev_down > $F; echo ev_down > $F; echo ev_down > $F; echo "aetrace 30" > $F
    echo ev0 > $F; echo "aetrace 30" > $F
    sleep 5
}
v 300 200 200 300
v 300 200 200 2000
v 300 200 2000 2000
v 150 100 100 150
v 600 400 400 600
echo "aespeed 200" > $F
echo done >> /tmp/aeexp.log
