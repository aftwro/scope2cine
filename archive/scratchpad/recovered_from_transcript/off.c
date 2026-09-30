#include <stddef.h>
#include <libavcodec/avcodec.h>
#define OFF(f) int off_##f = offsetof(AVCodecContext, f);
OFF(flags) OFF(width) OFF(height) OFF(coded_width) OFF(pix_fmt) OFF(thread_count) OFF(refcounted_frames) OFF(lowres) OFF(skip_frame) OFF(flags2) OFF(thread_type) OFF(skip_loop_filter) OFF(err_recognition) OFF(debug) OFF(skip_idct) OFF(hwaccel_flags) OFF(apply_cropping) OFF(extra_hw_frames)
