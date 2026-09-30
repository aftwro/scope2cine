#!/bin/sh
# Fail if a cross-built ARM binary needs a symbol (or symbol version) the device libraries lack.
# Usage: check_syms.sh <binary> [...]
R=$(cd "$(dirname "$0")/.." && pwd)/extracted/rootfs
avail=$(mktemp)
for l in "$R"/lib/*.so* "$R"/usr/lib/*.so*; do
    [ -f "$l" ] && arm-linux-gnueabihf-objdump -T "$l" 2>/dev/null
done | awk '$4 != "*UND*" && NF >= 7 {v=$(NF-1); gsub(/[()]/, "", v); print $NF "@" v; print $NF}' | sort -u > "$avail"
rc=0
for b in "$@"; do
    [ -f "$b" ] || { echo "$b: no such file"; rc=1; continue; }
    missing=$(arm-linux-gnueabihf-objdump -T "$b" | awk '/\*UND\*/ && $2 != "w" {v=$(NF-1); gsub(/[()]/, "", v); if (v ~ /^GLIBC_/) print $NF "@" v; else print $NF}' \
        | sort -u | comm -23 - "$avail")
    if [ -n "$missing" ]; then
        echo "$b: MISSING on device:"; echo "$missing" | sed 's/^/  /'; rc=1
    else
        echo "$b: all symbols present on device"
    fi
done
rm -f "$avail"
exit $rc
