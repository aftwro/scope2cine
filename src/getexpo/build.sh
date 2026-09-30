#!/bin/sh
# Cross-build against the device's own libraries (glibc 2.30) in extracted/rootfs.
# Vendor libs have no SONAME, so link with -L/-l to keep NEEDED as bare names.
set -e
cd "$(dirname "$0")"
R=$(cd ../.. && pwd)/extracted/rootfs
arm-linux-gnueabihf-gcc -O2 -Wall -std=gnu11 -fno-stack-protector -o getexpo getexpo.c \
  -nodefaultlibs -Wl,--no-as-needed $R/lib/libc.so.6 $R/lib/libm.so.6 $R/lib/libpthread.so.0 $R/lib/libdl.so.2 \
  -L$R/lib -lmi_sys -lmi_common -lcam_os_wrapper -lcam_fs_wrapper -lispalgo -lcus3a -lmi_isp -lgcc \
  -Wl,-rpath-link,$R/lib -Wl,-z,max-page-size=4096
arm-linux-gnueabihf-strip getexpo
