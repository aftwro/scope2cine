# per variant: set converge speeds, step -1 EV, trace, back to 0, trace
v() { echo aeconv 6 $1 > /tmp/camapp_fifo; echo aeconv 7 $2 > /tmp/camapp_fifo; echo aeconv 8 $3 > /tmp/camapp_fifo; echo aeconv 9 $4 > /tmp/camapp_fifo; sleep 2
 echo ev_down > /tmp/camapp_fifo; echo ev_down > /tmp/camapp_fifo; echo ev_down > /tmp/camapp_fifo; echo aetrace 30 > /tmp/camapp_fifo; echo ev0 > /tmp/camapp_fifo; echo aetrace 30 > /tmp/camapp_fifo; sleep 4; }
