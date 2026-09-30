#!/bin/sh
# Swap back to the previous camapp build (camapp <-> camapp.prev) and restart it.
D=/customer/camapp
[ -f $D/camapp.prev ] || { echo "rollback: no camapp.prev"; exit 1; }
mv $D/camapp $D/camapp.tmp && mv $D/camapp.prev $D/camapp && mv $D/camapp.tmp $D/camapp.prev && sync
pid=$(pidof camapp)
if [ -n "$pid" ]; then
    kill -TERM $pid
    i=0; while pidof camapp >/dev/null && [ $i -lt 20 ]; do sleep 1; i=$((i + 1)); done
fi
$D/camapp > /tmp/camapp.log 2>&1 &
echo "rollback: now running $(md5sum $D/camapp | cut -c1-32)"
