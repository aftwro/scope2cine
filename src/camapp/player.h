#ifndef CAMAPP_PLAYER_H
#define CAMAPP_PLAYER_H

/* Clip playback on the HDMI output, using the device's FFmpeg with SigmaStar's hardware decoders
 * (sshevc / ssh264 / ssmjpeg). Decoded NV12 frames are pushed into display layer 0 port 0; the
 * caller unbinds the live preview from that port before player_open() and rebinds it after
 * player_stop(). */

/* Start playing path in a thread. 0 on success. */
int player_open(const char *path);
/* Stop, join the thread and put the display port back to 1920x1080. Safe when not playing. */
void player_stop(void);
int player_active(void);
void player_toggle_pause(void);
/* Jump by seconds (negative = back), from the nearest keyframe. */
void player_seek(int seconds);
/* "REC_0002.ts  00:05 / 00:14  PAUSE" style line for the status bar. */
void player_status(char *buf, int size);

#endif
