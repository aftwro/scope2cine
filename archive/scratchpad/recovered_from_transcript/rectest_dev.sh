#!/bin/sh
# H.265 recording tests: 30p decimation, then 100 Mbps with a noisy image. Restores settings.
F=/tmp/camapp_fifo
echo "rec 1 30" > $F; sleep 10; echo "rec 0" > $F; sleep 3
echo "mbps 100" > $F; echo "shutter 16667" > $F; echo "gain 32768" > $F; echo "isp luma_nr 0" > $F; sleep 3
echo "rec 1 60" > $F; sleep 15; echo "rec 0" > $F; sleep 4
echo "mbps 40" > $F; echo "isp luma_nr -1" > $F; echo "shutter 500" > $F; echo "gain 1024" > $F; echo auto > $F
sleep 2; echo done > /tmp/rectest.done
