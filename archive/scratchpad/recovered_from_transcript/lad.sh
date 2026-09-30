for c in 0007 0009 0010 0008; do
  killall camapp; sleep 4; cd /; /customer/camapp/camapp > /tmp/camapp.log 2>&1 & sleep 15
  dmesg -c > /dev/null; echo play /app/bin/sd/REC_$c.ts > /tmp/camapp_fifo; sleep 8; echo playstop > /tmp/camapp_fifo; sleep 2
  echo REC_$c $(grep -a 'player: REC' /tmp/camapp.log | cut -d, -f2) last: $(grep -a 'player: frame' /tmp/camapp.log | tail -1 | cut -d' ' -f3) errMB $(dmesg | grep -c 'error MB') >> /tmp/lad.txt
done; echo DONE >> /tmp/lad.txt
