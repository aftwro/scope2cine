/* Minimal UVC gadget backend: feeds the latest JPEG from the encoder to the USB gadget.
 *
 * Kernel 4.9 f_uvc userspace protocol (private events, UVCIOC_SEND_RESPONSE), MJPEG only.
 * Descriptors come from our configfs gadget (uvc-gadget.sh): format 1 = MJPEG, frame 1 =
 * 1920x1080 (what camapp records), 30 or 60 fps, no still image. If that script fell back to
 * the vendor g_webcam (frames 1..7 up to a fake 3840x2160), every size still gets 1080p frames.
 */
#include "uvc.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/usb/ch9.h>
#include <linux/videodev2.h>
#include <pthread.h>
#include <sys/prctl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <time.h>
#include <unistd.h>

/* From drivers/usb/gadget/function/uvc.h (not exported as uapi in 4.9). */
#define UVC_EVENT_CONNECT    (V4L2_EVENT_PRIVATE_START + 0)
#define UVC_EVENT_DISCONNECT (V4L2_EVENT_PRIVATE_START + 1)
#define UVC_EVENT_STREAMON   (V4L2_EVENT_PRIVATE_START + 2)
#define UVC_EVENT_STREAMOFF  (V4L2_EVENT_PRIVATE_START + 3)
#define UVC_EVENT_SETUP      (V4L2_EVENT_PRIVATE_START + 4)
#define UVC_EVENT_DATA       (V4L2_EVENT_PRIVATE_START + 5)

struct uvc_request_data {
    int32_t length;
    uint8_t data[60];
};
#define UVCIOC_SEND_RESPONSE _IOW('U', 1, struct uvc_request_data)

/* UVC class requests and streaming selectors. */
enum { SET_CUR = 0x01, GET_CUR = 0x81, GET_MIN = 0x82, GET_MAX = 0x83, GET_RES = 0x84,
       GET_LEN = 0x85, GET_INFO = 0x86, GET_DEF = 0x87 };
enum { VS_PROBE_CONTROL = 1, VS_COMMIT_CONTROL = 2 };
#define STREAMING_INTERFACE 1

/* UVC 1.0 probe/commit control, 26 bytes. */
struct __attribute__((packed)) streaming_control {
    uint16_t bmHint;
    uint8_t bFormatIndex;
    uint8_t bFrameIndex;
    uint32_t dwFrameInterval;
    uint16_t wKeyFrameRate;
    uint16_t wPFrameRate;
    uint16_t wCompQuality;
    uint16_t wCompWindowSize;
    uint16_t wDelay;
    uint32_t dwMaxVideoFrameSize;
    uint32_t dwMaxPayloadTransferSize;
};

#define NUM_BUFS 3
#define BUF_SIZE (2 * 1024 * 1024)
#define MAX_PAYLOAD 3072            /* g_webcam streaming_maxpacket=3072 */

static int fd = -1;
static struct streaming_control probe, commit;
static int control_selector;        /* selector of the pending SET_CUR data stage */
static void *bufs[NUM_BUFS];
static uint32_t buf_len[NUM_BUFS];
static int streaming;
static volatile int run;
static pthread_t thread;

/* Latest encoded frame, written by the encoder thread. */
static pthread_mutex_t frame_lock = PTHREAD_MUTEX_INITIALIZER;
static uint8_t *frame;
static uint32_t frame_len;
long uvc_frames_sent;

void uvc_put_frame(const void *p1, uint32_t n1, const void *p2, uint32_t n2)
{
    if (n1 + n2 > BUF_SIZE || !streaming)        /* nobody watching: skip the copy */
        return;
    pthread_mutex_lock(&frame_lock);
    memcpy(frame, p1, n1);
    if (n2)
        memcpy(frame + n1, p2, n2);
    frame_len = n1 + n2;
    pthread_mutex_unlock(&frame_lock);
}

static void fill_control(struct streaming_control *c, int frame_index, uint32_t interval)
{
    memset(c, 0, sizeof(*c));
    c->bmHint = 1;
    c->bFormatIndex = 1;
    c->bFrameIndex = frame_index;
    c->dwFrameInterval = interval;
    c->dwMaxVideoFrameSize = BUF_SIZE;
    c->dwMaxPayloadTransferSize = MAX_PAYLOAD;
}

static void respond(const void *data, int len)
{
    struct uvc_request_data r = { .length = len };
    if (data && len > 0)            /* SET_CUR only announces the length of the data stage */
        memcpy(r.data, data, len);
    if (ioctl(fd, UVCIOC_SEND_RESPONSE, &r) < 0)
        printf("uvc: SEND_RESPONSE failed: %s\n", strerror(errno));
}

/* Controls on the video control interface (0). The kernel's configfs descriptors (read-only in
 * 4.9) advertise exactly two: the camera terminal (ID 1) auto-exposure mode and the processing
 * unit (ID 3) brightness. UVC 1.1 AE mode values: 1 manual, 2 auto, 4 shutter priority. */
#define CONTROL_INTERFACE 0
enum { CT_ID = 1, PU_ID = 3, CT_AE_MODE = 0x02, PU_BRIGHTNESS = 0x02 };   /* IDs from lsusb -v */
static int ctrl_pending = -1;       /* control of the pending SET_CUR data stage, -1 = streaming */

static void handle_control(const struct usb_ctrlrequest *req, int entity, int cs)
{
    int ctrl = entity == CT_ID && cs == CT_AE_MODE ? UVC_CTRL_AE_MODE
               : entity == PU_ID && cs == PU_BRIGHTNESS ? UVC_CTRL_BRIGHTNESS : -1;
    if (ctrl < 0) {
        respond(NULL, -EL2HLT);
        return;
    }
    if (ctrl == UVC_CTRL_AE_MODE) {             /* 1 byte bitmap */
        uint8_t v = 0;
        switch (req->bRequest) {
        case SET_CUR: ctrl_pending = ctrl; respond(NULL, 1); return;
        case GET_CUR: v = (uint8_t)uvc_control_get(ctrl); break;
        case GET_RES: v = 0x07; break;          /* manual, auto, shutter priority */
        case GET_DEF: v = 0x02; break;
        case GET_INFO: v = 3; break;
        default: respond(NULL, -EL2HLT); return;
        }
        respond(&v, 1);
        return;
    }
    int16_t v = 0;                              /* brightness: signed 16 bit, EV comp in 1/3 stops */
    switch (req->bRequest) {
    case SET_CUR: ctrl_pending = ctrl; respond(NULL, 2); return;
    case GET_CUR: v = (int16_t)uvc_control_get(ctrl); break;
    case GET_MIN: v = -9; break;
    case GET_MAX: v = 9; break;
    case GET_RES: v = 1; break;
    case GET_DEF: v = 0; break;
    case GET_INFO: { uint8_t info = 3; respond(&info, 1); return; }
    default: respond(NULL, -EL2HLT); return;
    }
    respond(&v, 2);
}

static void handle_setup(const struct usb_ctrlrequest *req)
{
    printf("uvc: setup bRequestType %02x bRequest %02x wValue %04x wIndex %04x wLength %u\n",
           req->bRequestType, req->bRequest, req->wValue, req->wIndex, req->wLength);
    int intf = req->wIndex & 0xff;
    int cs = req->wValue >> 8;
    struct streaming_control c;

    if (intf == CONTROL_INTERFACE && (req->bRequestType & 0x1f) == 1) {   /* recipient: interface */
        handle_control(req, req->wIndex >> 8, cs);
        return;
    }
    if (intf != STREAMING_INTERFACE || (cs != VS_PROBE_CONTROL && cs != VS_COMMIT_CONTROL)) {
        respond(NULL, -EL2HLT);     /* stall: not implemented */
        return;
    }
    switch (req->bRequest) {
    case SET_CUR:
        ctrl_pending = -1;
        control_selector = cs;
        respond(NULL, sizeof(c));   /* host sends the data stage next */
        break;
    case GET_CUR:
        respond(cs == VS_PROBE_CONTROL ? &probe : &commit, sizeof(c));
        break;
    case GET_MIN:
    case GET_DEF:
        fill_control(&c, 1, 333333);
        respond(&c, sizeof(c));
        break;
    case GET_MAX:
        fill_control(&c, 1, 166666);
        respond(&c, sizeof(c));
        break;
    case GET_RES:
        memset(&c, 0, sizeof(c));
        respond(&c, sizeof(c));
        break;
    case GET_LEN: {
        uint16_t len = sizeof(c);
        respond(&len, 2);
        break;
    }
    case GET_INFO: {
        uint8_t info = 3;           /* supports GET and SET */
        respond(&info, 1);
        break;
    }
    default:
        respond(NULL, -EL2HLT);
    }
}

static void handle_data(const struct uvc_request_data *d)
{
    if (ctrl_pending >= 0) {                    /* data stage of a camera control SET_CUR */
        int v = ctrl_pending == UVC_CTRL_AE_MODE ? d->data[0] : (int16_t)(d->data[0] | d->data[1] << 8);
        printf("uvc: control %d set to %d by the host\n", ctrl_pending, v);
        uvc_control_set(ctrl_pending, v);
        ctrl_pending = -1;
        return;
    }
    struct streaming_control *target = control_selector == VS_COMMIT_CONTROL ? &commit : &probe;
    struct streaming_control in;
    memset(&in, 0, sizeof(in));
    memcpy(&in, d->data, d->length < (int)sizeof(in) ? d->length : (int)sizeof(in));
    int idx = in.bFrameIndex >= 1 && in.bFrameIndex <= 7 ? in.bFrameIndex : 1;
    fill_control(target, idx, in.dwFrameInterval ? in.dwFrameInterval : 333333);
    if (control_selector == VS_COMMIT_CONTROL)
        printf("uvc: commit frame %d, interval %u (sending 1920x1080)\n", idx, target->dwFrameInterval);
}

static int queue(int index)
{
    struct v4l2_buffer b = { .index = index, .type = V4L2_BUF_TYPE_VIDEO_OUTPUT, .memory = V4L2_MEMORY_MMAP };
    pthread_mutex_lock(&frame_lock);
    uint32_t n = frame_len <= buf_len[index] ? frame_len : 0;   /* never overrun the gadget buffer */
    memcpy(bufs[index], frame, n);
    b.bytesused = n;
    pthread_mutex_unlock(&frame_lock);
    return ioctl(fd, VIDIOC_QBUF, &b);
}

static void stream_on(void)
{
    int idx = commit.bFrameIndex ? commit.bFrameIndex : 4;
    struct v4l2_format fmt = { .type = V4L2_BUF_TYPE_VIDEO_OUTPUT };
    fmt.fmt.pix.width = 1920;
    fmt.fmt.pix.height = 1080;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
    fmt.fmt.pix.field = V4L2_FIELD_NONE;
    fmt.fmt.pix.sizeimage = BUF_SIZE;
    if (ioctl(fd, VIDIOC_S_FMT, &fmt) < 0)
        printf("uvc: S_FMT failed: %s\n", strerror(errno));

    struct v4l2_requestbuffers rb = { .count = NUM_BUFS, .type = V4L2_BUF_TYPE_VIDEO_OUTPUT,
                                      .memory = V4L2_MEMORY_MMAP };
    if (ioctl(fd, VIDIOC_REQBUFS, &rb) < 0 || rb.count < NUM_BUFS) {
        printf("uvc: REQBUFS failed: %s (count %u)\n", strerror(errno), rb.count);
        return;
    }
    for (int i = 0; i < NUM_BUFS; i++) {
        struct v4l2_buffer b = { .index = i, .type = V4L2_BUF_TYPE_VIDEO_OUTPUT, .memory = V4L2_MEMORY_MMAP };
        ioctl(fd, VIDIOC_QUERYBUF, &b);
        buf_len[i] = b.length;
        printf("uvc: buffer %d length %u\n", i, b.length);
        bufs[i] = mmap(NULL, b.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, b.m.offset);
        if (bufs[i] == MAP_FAILED) {
            printf("uvc: mmap failed: %s\n", strerror(errno));
            return;
        }
        queue(i);
    }
    int type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    if (ioctl(fd, VIDIOC_STREAMON, &type) < 0)
        printf("uvc: STREAMON failed: %s\n", strerror(errno));
    streaming = 1;
    printf("uvc: streaming 1920x1080 (host frame %d)\n", idx);
}

static void stream_off(void)
{
    if (!streaming)
        return;
    int type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    ioctl(fd, VIDIOC_STREAMOFF, &type);
    for (int i = 0; i < NUM_BUFS; i++)
        if (bufs[i] && bufs[i] != MAP_FAILED)
            munmap(bufs[i], buf_len[i]);
    struct v4l2_requestbuffers rb = { .count = 0, .type = V4L2_BUF_TYPE_VIDEO_OUTPUT,
                                      .memory = V4L2_MEMORY_MMAP };
    ioctl(fd, VIDIOC_REQBUFS, &rb);
    streaming = 0;
    printf("uvc: stream off (%ld frames sent)\n", uvc_frames_sent);
}

static void handle_event(void)
{
    struct v4l2_event ev;
    if (ioctl(fd, VIDIOC_DQEVENT, &ev) < 0)
        return;
    printf("uvc: event 0x%x\n", ev.type - V4L2_EVENT_PRIVATE_START);
    switch (ev.type) {
    case UVC_EVENT_CONNECT:
        printf("uvc: host connected\n");
        break;
    case UVC_EVENT_DISCONNECT:
        printf("uvc: host disconnected\n");
        stream_off();
        break;
    case UVC_EVENT_SETUP:
        handle_setup((struct usb_ctrlrequest *)ev.u.data);
        break;
    case UVC_EVENT_DATA:
        handle_data((struct uvc_request_data *)ev.u.data);
        break;
    case UVC_EVENT_STREAMON:
        stream_on();
        break;
    case UVC_EVENT_STREAMOFF:
        stream_off();
        break;
    }
}

static void *uvc_thread(void *arg)
{
    prctl(PR_SET_NAME, "uvc");                 /* thread name for top / cpustat */
    int64_t next_ns = 0;                    /* monotonic time the next frame is due */
    while (run) {
        fd_set efds, wfds;
        struct timeval tv = { 0, 200000 };
        FD_ZERO(&efds);
        FD_ZERO(&wfds);
        FD_SET(fd, &efds);
        if (streaming)
            FD_SET(fd, &wfds);
        if (select(fd + 1, NULL, &wfds, &efds, &tv) <= 0)
            continue;
        if (FD_ISSET(fd, &efds))
            handle_event();
        if (streaming && FD_ISSET(fd, &wfds)) {
            struct v4l2_buffer b = { .type = V4L2_BUF_TYPE_VIDEO_OUTPUT, .memory = V4L2_MEMORY_MMAP };
            if (ioctl(fd, VIDIOC_DQBUF, &b) != 0)
                continue;
            /* Pace to the frame interval the host committed (30 or 60 fps): the buffer comes back as
             * soon as USB sent it, which let camapp push ~120 frames/s the host never asked for
             * (~27% of a core in USB work, measured 2026-09-26). */
            struct timespec now;
            clock_gettime(CLOCK_MONOTONIC, &now);
            int64_t now_ns = now.tv_sec * 1000000000LL + now.tv_nsec;
            int64_t step = (int64_t)(commit.dwFrameInterval ? commit.dwFrameInterval : 333333) * 100;
            if (next_ns > now_ns) {
                struct timespec w = { (next_ns - now_ns) / 1000000000LL, (next_ns - now_ns) % 1000000000LL };
                nanosleep(&w, NULL);
            }
            next_ns = (next_ns > now_ns - step ? next_ns : now_ns) + step;   /* no catch-up burst */
            if (queue(b.index) == 0)
                uvc_frames_sent++;
        }
    }
    return NULL;
}

/* Soft-disconnect and reconnect the USB device, so a host that was talking to an earlier camapp
 * (restart after a mode or crop change) enumerates again instead of polling a dead session. */
static void usb_reconnect(void)
{
    DIR *d = opendir("/sys/class/udc");
    struct dirent *e;
    char path[128] = "";
    while (d && (e = readdir(d)))
        if (e->d_name[0] != '.')
            snprintf(path, sizeof(path), "/sys/class/udc/%s/soft_connect", e->d_name);
    if (d)
        closedir(d);
    FILE *f = path[0] ? fopen(path, "w") : NULL;
    if (!f)
        return;
    fputs("disconnect", f);
    fclose(f);
    usleep(300000);
    f = fopen(path, "w");
    if (f) {
        fputs("connect", f);
        fclose(f);
    }
    printf("uvc: USB reconnected (%s)\n", path);
}

int uvc_start(const char *dev)
{
    frame = malloc(BUF_SIZE);
    if (!frame)
        return -1;
    fd = open(dev, O_RDWR | O_NONBLOCK);     /* opening the node activates the USB function */
    if (fd < 0) {
        printf("uvc: open %s failed: %s\n", dev, strerror(errno));
        return -1;
    }
    const uint32_t events[] = { UVC_EVENT_CONNECT, UVC_EVENT_DISCONNECT, UVC_EVENT_SETUP,
                                UVC_EVENT_DATA, UVC_EVENT_STREAMON, UVC_EVENT_STREAMOFF };
    for (unsigned i = 0; i < sizeof(events) / sizeof(events[0]); i++) {
        struct v4l2_event_subscription sub = { .type = events[i] };
        ioctl(fd, VIDIOC_SUBSCRIBE_EVENT, &sub);
    }
    fill_control(&probe, 1, 333333);
    commit = probe;
    usb_reconnect();
    run = 1;
    return pthread_create(&thread, NULL, uvc_thread, NULL);
}

void uvc_stop(void)
{
    if (fd < 0)
        return;
    run = 0;
    pthread_join(thread, NULL);
    stream_off();
    close(fd);
    fd = -1;
    free(frame);
}
