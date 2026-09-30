#!/bin/sh
# cpustat.sh SECS: whole-CPU split and per-thread CPU of camapp over SECS seconds (2 cores).
# Push to the camera: tools/uart_push.py tools/device/cpustat.sh /tmp/cpustat.sh
S=${1:-5}; P=$(pidof camapp)
snap() {
    head -1 /proc/stat
    for t in /proc/$P/task/*; do echo "T $(cat $t/comm | tr ' ' _) $(cut -d' ' -f14,15 $t/stat)"; done
    for t in /proc/[0-9]*; do [ -r $t/stat ] && awk '{print "K",$2,$14+$15}' $t/stat 2>/dev/null; done
}
snap > /tmp/cs1; sleep $S; snap > /tmp/cs2
awk -v S=$S 'FNR==1 && /^cpu /{ if (NR==FNR) {for(i=2;i<=11;i++) a[i]=$i} else {for(i=2;i<=11;i++){d[i]=$i-a[i]; tot+=d[i]} printf "CPU total (2 cores): user %.1f%% sys %.1f%% irq %.1f%% idle %.1f%% iowait %.1f%%\n", 100*(d[2]+d[3])/tot, 100*d[4]/tot, 100*(d[7]+d[8])/tot, 100*d[5]/tot, 100*d[6]/tot} next}
 $1=="T"{ if (NR==FNR) t1[$2]+=$3+$4; else t2[$2]+=$3+$4; next }
 $1=="K"{ if (NR==FNR) k1[$2]=$3; else k2[$2]=$3; next }
 END { print "camapp threads (% of one core):"; for (n in t2) { v=(t2[n]-t1[n])/S; if (v>0.5) printf "  %-18s %5.1f\n", n, v }
       print "busiest processes/kernel threads (% of one core):"; for (n in k2) { v=(k2[n]-k1[n])/S; if (v>1) printf "  %-22s %5.1f\n", n, v } }' /tmp/cs1 /tmp/cs2
