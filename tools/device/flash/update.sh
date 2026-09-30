#!/bin/sh
# Install a new camapp build and restart it in place, with automatic rollback.
# Usage: update.sh <new camapp binary>
# Keeps the running build as camapp.prev (atomic renames, so a power cut leaves
# either the old or the new build in place). If the new build is not healthy
# after 15 s it is moved to camapp.bad and camapp.prev is restored.
D=/customer/camapp
NEW=$1
[ -f "$NEW" ] || { echo "update: no such file $NEW"; exit 1; }

healthy() {   # camapp alive and finished bring-up (uvc_start is the last setup step)
    pidof camapp >/dev/null && grep -q "^uvc_start" /tmp/camapp.log
}
restart() {
    pid=$(pidof camapp)
    if [ -n "$pid" ]; then
        kill -TERM $pid
        i=0; while pidof camapp >/dev/null && [ $i -lt 20 ]; do sleep 1; i=$((i + 1)); done
    fi
    $D/camapp > /tmp/camapp.log 2>&1 &
}

cp "$NEW" $D/camapp.new && chmod +x $D/camapp.new && sync || { echo "update: copy failed"; exit 1; }
mv $D/camapp $D/camapp.prev && mv $D/camapp.new $D/camapp && sync
echo "update: installed $(md5sum $D/camapp | cut -c1-32), restarting"
restart
sleep 15
if healthy; then
    echo "update: OK, new build running (pid $(pidof camapp))"
    exit 0
fi
echo "update: new build NOT healthy, rolling back"
tail -5 /tmp/camapp.log
kill -9 $(pidof camapp) 2>/dev/null
mv $D/camapp $D/camapp.bad && mv $D/camapp.prev $D/camapp && sync
restart
sleep 15
if healthy; then echo "update: ROLLED BACK, previous build running"; else echo "update: ROLLBACK FAILED, reboot to use boot fallback"; fi
exit 2
