/* mitrace: LD_PRELOAD shim that logs MI_* pipeline bring-up calls made by cardv.
 *
 * Each wrapper passes 8 argument words straight through to the real function
 * (4 in registers + 4 on the stack under ARM EABI), so functions with up to
 * 8 words of arguments are forwarded unchanged. Arguments that look like
 * pointers are hex-dumped (first DUMP_LEN bytes) after a readability probe,
 * so a bad pointer cannot crash the host process.
 */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#define DUMP_LEN 512

typedef uint32_t w;
typedef int (*fn8)(w, w, w, w, w, w, w, w);

static int log_fd = -1;
static int probe[2] = { -1, -1 };
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

static void out(const char *fmt, ...)
{
    char buf[8192];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (log_fd >= 0 && n > 0)
        write(log_fd, buf, n < (int)sizeof(buf) ? n : (int)sizeof(buf) - 1);
}

/* True if [p, p+len) is readable: write() into a pipe fails with EFAULT otherwise. */
static int readable(w p, size_t len)
{
    char sink[DUMP_LEN];
    if (write(probe[1], (const void *)p, len) != (ssize_t)len)
        return 0;
    read(probe[0], sink, len);
    return 1;
}

static void dump(int i, w p)
{
    if (p < 0x10000 || !readable(p, DUMP_LEN))
        return;
    const uint8_t *b = (const uint8_t *)p;
    out("    a%d@%08x:", i, p);
    for (int k = 0; k < DUMP_LEN; k += 4)
        out(" %08x", b[k] | b[k + 1] << 8 | b[k + 2] << 16 | (uint32_t)b[k + 3] << 24);
    out("\n");
}

static int call(const char *name, fn8 *real, w a0, w a1, w a2, w a3, w a4, w a5, w a6, w a7)
{
    if (!*real)
        *real = (fn8)dlsym(RTLD_NEXT, name);
    w a[8] = { a0, a1, a2, a3, a4, a5, a6, a7 };
    struct timeval tv;
    gettimeofday(&tv, NULL);

    pthread_mutex_lock(&lock);
    out("[%d] %ld.%03ld %s(%x, %x, %x, %x, %x, %x, %x, %x)\n", (int)getpid(), (long)tv.tv_sec, (long)tv.tv_usec / 1000,
        name, a0, a1, a2, a3, a4, a5, a6, a7);
    for (int i = 0; i < 8; i++)
        dump(i, a[i]);
    pthread_mutex_unlock(&lock);

    int r = *real ? (*real)(a0, a1, a2, a3, a4, a5, a6, a7) : -1;

    pthread_mutex_lock(&lock);
    out("  [%d] -> %s = %d (0x%x)\n", (int)getpid(), name, r, r);
    for (int i = 0; i < 8; i++)     /* outputs (Get* calls fill structs) */
        if (strstr(name, "Get") || strstr(name, "Query"))
            dump(i, a[i]);
    pthread_mutex_unlock(&lock);
    return r;
}

#define WRAP(name)                                                                   \
    int name(w a0, w a1, w a2, w a3, w a4, w a5, w a6, w a7)                         \
    {                                                                                \
        static fn8 real;                                                             \
        return call(#name, &real, a0, a1, a2, a3, a4, a5, a6, a7);                   \
    }

__attribute__((constructor)) static void init(void)
{
    /* RAM only: cardv remounts the SD card at startup, which strands open files there. */
    log_fd = open("/tmp/mi_trace.log", O_WRONLY | O_CREAT | O_APPEND, 0644);
    pipe(probe);
    out("=== mitrace loaded, pid %d ===\n", getpid());
}

/* sensor */
WRAP(MI_SNR_InitDev) WRAP(MI_SNR_DeInitDev) WRAP(MI_SNR_Enable) WRAP(MI_SNR_Disable)
WRAP(MI_SNR_GetPadInfo) WRAP(MI_SNR_GetPlaneInfo) WRAP(MI_SNR_GetRes) WRAP(MI_SNR_QueryResCount)
WRAP(MI_SNR_SetFps) WRAP(MI_SNR_SetPlaneMode) WRAP(MI_SNR_SetRes)
/* VIF */
WRAP(MI_VIF_CreateDevGroup) WRAP(MI_VIF_CreateDevGroupExt) WRAP(MI_VIF_DestroyDevGroup)
WRAP(MI_VIF_EnableDev) WRAP(MI_VIF_DisableDev) WRAP(MI_VIF_SetDevAttr)
WRAP(MI_VIF_EnableOutputPort) WRAP(MI_VIF_DisableOutputPort)
WRAP(MI_VIF_SetOutputPortAttr) WRAP(MI_VIF_GetOutputPortAttr)
/* ISP pipeline (no 3A / IQ) */
WRAP(MI_ISP_CreateDevice) WRAP(MI_ISP_DestoryDevice) WRAP(MI_ISP_CreateChannel) WRAP(MI_ISP_DestroyChannel)
WRAP(MI_ISP_SetChnParam) WRAP(MI_ISP_SetInputPortCrop) WRAP(MI_ISP_SetOutputPortParam)
WRAP(MI_ISP_EnableOutputPort) WRAP(MI_ISP_DisableOutputPort) WRAP(MI_ISP_StartChannel) WRAP(MI_ISP_StopChannel)
WRAP(MI_ISP_API_CmdLoadBinFile)
/* SCL */
WRAP(MI_SCL_CreateDevice) WRAP(MI_SCL_DestroyDevice) WRAP(MI_SCL_CreateChannel) WRAP(MI_SCL_DestroyChannel)
WRAP(MI_SCL_SetChnParam) WRAP(MI_SCL_SetInputPortCrop) WRAP(MI_SCL_SetOutputPortParam) WRAP(MI_SCL_GetOutputPortParam)
WRAP(MI_SCL_EnableOutputPort) WRAP(MI_SCL_DisableOutputPort) WRAP(MI_SCL_StartChannel) WRAP(MI_SCL_StopChannel)
/* SYS setup (no per-frame buffer calls) */
WRAP(MI_SYS_Init) WRAP(MI_SYS_Exit) WRAP(MI_SYS_BindChnPort2) WRAP(MI_SYS_UnBindChnPort)
WRAP(MI_SYS_SetChnOutputPortDepth) WRAP(MI_SYS_GetVersion)
/* display */
WRAP(MI_DISP_InitDev) WRAP(MI_DISP_DeInitDev) WRAP(MI_DISP_SetPubAttr) WRAP(MI_DISP_Enable) WRAP(MI_DISP_Disable)
WRAP(MI_DISP_BindVideoLayer) WRAP(MI_DISP_UnBindVideoLayer) WRAP(MI_DISP_SetVideoLayerAttr)
WRAP(MI_DISP_EnableVideoLayer) WRAP(MI_DISP_DisableVideoLayer)
WRAP(MI_DISP_SetInputPortAttr) WRAP(MI_DISP_GetInputPortAttr) WRAP(MI_DISP_EnableInputPort) WRAP(MI_DISP_DisableInputPort)
WRAP(MI_DISP_SetInputPortSyncMode) WRAP(MI_DISP_ShowInputPort) WRAP(MI_DISP_HideInputPort)
WRAP(MI_DISP_SetZoomInWindow) WRAP(MI_DISP_SetWBCSource) WRAP(MI_DISP_SetWBCAttr) WRAP(MI_DISP_EnableWBC) WRAP(MI_DISP_DisableWBC)
/* HDMI */
WRAP(MI_HDMI_Init) WRAP(MI_HDMI_DeInit) WRAP(MI_HDMI_Open) WRAP(MI_HDMI_Close) WRAP(MI_HDMI_SetAttr) WRAP(MI_HDMI_GetAttr)
WRAP(MI_HDMI_GetSinkInfo) WRAP(MI_HDMI_Start) WRAP(MI_HDMI_Stop) WRAP(MI_HDMI_SetAvMute)
/* encoder setup (no GetStream/ReleaseStream/Query per-frame calls) */
WRAP(MI_VENC_CreateDev) WRAP(MI_VENC_DestroyDev) WRAP(MI_VENC_CreateChn) WRAP(MI_VENC_DestroyChn) WRAP(MI_VENC_ResetChn)
WRAP(MI_VENC_SetChnAttr) WRAP(MI_VENC_GetChnAttr) WRAP(MI_VENC_SetRcParam) WRAP(MI_VENC_GetRcParam)
WRAP(MI_VENC_SetInputSourceConfig) WRAP(MI_VENC_SetMaxStreamCnt) WRAP(MI_VENC_SetRefParam) WRAP(MI_VENC_SetSuperFrameCfg)
WRAP(MI_VENC_SetRcPriority) WRAP(MI_VENC_StartRecvPic) WRAP(MI_VENC_StopRecvPic) WRAP(MI_VENC_RequestIdr)
/* OSD regions (no per-frame canvas updates) */
WRAP(MI_RGN_InitDev) WRAP(MI_RGN_DeInitDev) WRAP(MI_RGN_Create) WRAP(MI_RGN_Destroy)
WRAP(MI_RGN_AttachToChn) WRAP(MI_RGN_DetachFromChn)
/* panel */
WRAP(MI_PANEL_InitDev) WRAP(MI_PANEL_DeInitDev)
