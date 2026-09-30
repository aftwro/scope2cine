#!/bin/sh
# Shared object for LD_PRELOAD into cardv; built against the device's glibc 2.30.
set -e
cd "$(dirname "$0")"
R=$(cd ../.. && pwd)/extracted/rootfs
arm-linux-gnueabihf-gcc -O2 -Wall -std=gnu11 -U_TIME_BITS -D_TIME_BITS=32 -U_FILE_OFFSET_BITS -D_FILE_OFFSET_BITS=32 -fno-stack-protector -fPIC -shared -o mitrace.so mitrace.c \
  -nodefaultlibs -Wl,--no-as-needed $R/lib/libc.so.6 $R/lib/libdl.so.2 $R/lib/libpthread.so.0 -lgcc \
  -Wl,-z,max-page-size=4096
arm-linux-gnueabihf-strip mitrace.so
