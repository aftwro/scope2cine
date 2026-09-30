#!/bin/sh
# Cross-build against the device's own libraries (glibc 2.30) in extracted/rootfs.
set -e
cd "$(dirname "$0")"                               # sources are next to this script
D=$(cd ../.. && pwd)                               # repository root
R=$D/extracted/rootfs                              # device root filesystem (see docs/building.md)
F=$D/third_party/ffmpeg-4.1.3                      # FFmpeg 4.1.3 headers (device libs are SigmaStar's build)
arm-linux-gnueabihf-gcc -O2 -Wall -Wno-attributes -std=gnu11 -I$F -U_TIME_BITS -D_TIME_BITS=32 -U_FILE_OFFSET_BITS -D_FILE_OFFSET_BITS=32 \
  -fno-stack-protector -o camapp camapp.c uvc.c avi.c ts.c isp.c rec.c menu.c cmd.c scopes.c player.c input.c settings.c osd.c sensordbg.c dng.c rgn.c \
  -nodefaultlibs -Wl,--no-as-needed $R/lib/libc.so.6 $R/lib/libm.so.6 $R/lib/libpthread.so.0 $R/lib/libdl.so.2 \
  -L$R/lib -lmi_sys -lmi_common -lcam_os_wrapper -lcam_fs_wrapper -lispalgo -lcus3a \
  -lmi_sensor -lmi_vif -lmi_isp -lmi_scl -lmi_disp -lmi_hdmi -lmi_venc -lmi_rgn \
  -lavformat -lavcodec -lavutil -lswresample -lfdk-aac -lmi_vdec -lmi_jpd -lgcc \
  -Wl,-rpath-link,$R/lib -Wl,-z,max-page-size=4096
arm-linux-gnueabihf-strip camapp
