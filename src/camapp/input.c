/* Button + IR remote input -> action names (see input.h).
 * Codes captured 2026-09-24: body buttons are EV_KEY codes on event0; the remote
 * reports raw NEC scancodes as EV_MSC/MSC_SCAN on event1 (the kernel keymap only
 * translates 5 of its 18 buttons, so scancodes are used). */
#include "input.h"

#include <fcntl.h>
#include <linux/input.h>
#include <pthread.h>
#include <sys/prctl.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>

struct map { unsigned code; const char *name; const char *action; };

/* Body buttons: EV_KEY codes. Actions without an effect yet are logged only. */
static const struct map body[] = {
    { 103, "UP", "up" }, { 108, "DOWN", "down" }, { 105, "AE", "ae" },
    { 139, "MENU", "menu" }, { 28, "OK", "ok" },
};

/* IR remote: NEC scancodes. */
static const struct map remote[] = {
    { 0x45, "zoom+", "ev_up" }, { 0x46, "up", "up" }, { 0x47, "zoom-", "ev_down" },
    { 0x44, "left", "left" }, { 0x40, "MENU", "menu" }, { 0x43, "right", "right" },
    { 0x07, "mirror", "loupe" }, { 0x15, "down", "down" }, { 0x09, "AE", "remote_ae" },
    { 0x16, "flip", "peaking" }, { 0x19, "+", "metering_next" }, { 0x0d, "AWB", "awb" },
    { 0x0c, "contrast", "ae_lock" }, { 0x18, "freeze", "histogram_toggle" }, { 0x5e, "play/pause", "reset" },
    { 0x08, "photo", "snap" }, { 0x1c, "record", "rec_toggle" }, { 0x5a, "gallery", "overlay_next" },
};

/* Holding a key: the remote sends NEC repeat frames every ~108 ms (each one arrives as another
 * MSC_SCAN). The body buttons (adc-keys) have no kernel autorepeat, so a held body key is
 * repeated here every BODY_PERIOD_MS until its release event. Only
 * navigation / stepping actions repeat; photo, record and the rest fire once per press. */
#define NEW_PRESS_MS 120       /* a scan after a longer gap (or a different code) is a new press; NEC repeat frames come every ~108 ms */
#define HOLD_DELAY_MS 400      /* hold this long before repeating */
#define FAST_AFTER_MS 2000     /* after this, each repeat steps 4 times */
#define BODY_PERIOD_MS 110
#define LONG_PRESS_MS 800      /* body OK held this long = "reset" (menu: back to Auto/Default) */
static const struct map ok_long = { 28, "OK (hold)", "reset" };
/* Body MENU gestures (user 2026-09-29): a single press = "menu_short" on release (photo, at once),
 * held LONG_PRESS_MS = "menu_long" (open / close the menu), a 2nd press within CLICK_GAP_MS of the
 * previous release = "menu_double" (OSD hide), a 3rd = "menu_triple" (record). Press 1 of a double
 * or triple has already fired its photo (accepted: photo without delay). The double waits
 * CLICK_GAP_MS for a possible 3rd press, so a triple no longer also toggles the OSD (2026-09-30). */
#define CLICK_GAP_MS 450
static const struct map menu_short = { 139, "MENU", "menu_short" }, menu_long = { 139, "MENU (hold)", "menu_long" },
                        menu_double = { 139, "MENU x2", "menu_double" }, menu_triple = { 139, "MENU x3", "menu_triple" };

static int fd_body = -1, fd_ir = -1;
static volatile int run;
static pthread_t thread;
static void (*action_cb)(const char *);

static const struct map *lookup(const struct map *m, int n, unsigned code)
{
    for (int i = 0; i < n; i++)
        if (m[i].code == code)
            return &m[i];
    return NULL;
}

static void fire(const char *src, const struct map *m, unsigned code)
{
    if (!m) {
        printf("input: %s unknown code 0x%x\n", src, code);
        return;
    }
    printf("input: %s %s -> %s\n", src, m->name, m->action);
    action_cb(m->action);
}

static long ms(const struct timeval *tv) { return tv->tv_sec * 1000L + tv->tv_usec / 1000; }

static int repeats(const struct map *m)
{
    static const char *const names[] = { "up", "down", "left", "right", "ev_up", "ev_down" };
    for (unsigned i = 0; m && i < sizeof(names) / sizeof(names[0]); i++)
        if (!strcmp(m->action, names[i]))
            return 1;
    return 0;
}

/* One repeat of a held key: 1 step, 4 once held long. */
static void fire_held(const char *src, const struct map *m, unsigned code, long held_ms)
{
    if (!repeats(m) || held_ms < HOLD_DELAY_MS)
        return;
    for (int i = held_ms >= FAST_AFTER_MS ? 4 : 1; i > 0; i--)
        fire(src, m, code);
}

static void *input_thread(void *arg)
{
    prctl(PR_SET_NAME, "input");                 /* thread name for top / cpustat */
    unsigned last_code = 0;
    long last_ms = 0, press_ms = 0, body_press_ms = 0, body_next_ms = 0;
    const struct map *body_held = NULL;
    int ok_down = 0, ok_long_done = 0;          /* body OK: short press = ok on release, long = reset */
    int menu_down = 0, menu_long_done = 0, menu_clicks = 0;
    long menu_release_ms = 0, menu_double_at = 0;  /* pending double click fires at this time, 0 = none */
    while (run) {
        fd_set fds;
        struct timeval tv = { 0, body_held || ok_down || menu_double_at ? BODY_PERIOD_MS * 1000 / 2 : 200000 };
        FD_ZERO(&fds);
        FD_SET(fd_body, &fds);
        FD_SET(fd_ir, &fds);
        int maxfd = fd_body > fd_ir ? fd_body : fd_ir;
        int ready = select(maxfd + 1, &fds, NULL, NULL, &tv);
        if (body_held) {
            struct timeval now;
            gettimeofday(&now, NULL);
            if (ms(&now) >= body_next_ms) {
                fire_held("button", body_held, body_held->code, ms(&now) - body_press_ms);
                body_next_ms = ms(&now) + BODY_PERIOD_MS;
            }
        }
        if (ok_down && !ok_long_done) {
            struct timeval now;
            gettimeofday(&now, NULL);
            if (ms(&now) - body_press_ms >= LONG_PRESS_MS) {
                fire("button", &ok_long, ok_long.code);
                ok_long_done = 1;
            }
        }
        if (menu_down && !menu_long_done) {
            struct timeval now;
            gettimeofday(&now, NULL);
            if (ms(&now) - body_press_ms >= LONG_PRESS_MS) {
                fire("button", &menu_long, menu_long.code);
                menu_long_done = 1;
                menu_clicks = 0;
            }
        }
        if (menu_double_at) {
            struct timeval now;
            gettimeofday(&now, NULL);
            if (ms(&now) >= menu_double_at) {         /* no 3rd press came: it was a double click */
                fire("button", &menu_double, menu_double.code);
                menu_double_at = 0;
                menu_clicks = 0;
            }
        }
        if (ready <= 0)
            continue;
        struct input_event ev;
        if (FD_ISSET(fd_body, &fds) && read(fd_body, &ev, sizeof(ev)) == sizeof(ev) && ev.type == EV_KEY) {
            const struct map *m = lookup(body, sizeof(body) / sizeof(body[0]), ev.code);
            if (ev.value == 1) {
                body_press_ms = ms(&ev.time);
                body_next_ms = body_press_ms + HOLD_DELAY_MS;
                body_held = repeats(m) ? m : NULL;
                if (ev.code == ok_long.code) {      /* decided on release or after LONG_PRESS_MS */
                    ok_down = 1;
                    ok_long_done = 0;
                } else if (ev.code == menu_short.code) {   /* decided on release or after LONG_PRESS_MS */
                    menu_down = 1;
                    menu_long_done = 0;
                    if (body_press_ms - menu_release_ms > CLICK_GAP_MS)
                        menu_clicks = 0;
                } else {
                    fire("button", m, ev.code);
                }
            } else if (ev.value == 0) {
                body_held = NULL;
                if (ev.code == ok_long.code && ok_down && !ok_long_done)
                    fire("button", m, ev.code);
                if (ev.code == ok_long.code)
                    ok_down = 0;
                if (ev.code == menu_short.code && menu_down && !menu_long_done) {
                    menu_clicks++;
                    menu_release_ms = ms(&ev.time);
                    if (menu_clicks == 1) {
                        fire("button", &menu_short, ev.code);
                    } else if (menu_clicks == 2) {
                        menu_double_at = menu_release_ms + CLICK_GAP_MS;   /* decided later: double or triple */
                    } else {
                        menu_double_at = 0;
                        fire("button", &menu_triple, ev.code);
                        menu_clicks = 0;
                    }
                }
                if (ev.code == menu_short.code)
                    menu_down = 0;
            }
        }
        if (FD_ISSET(fd_ir, &fds) && read(fd_ir, &ev, sizeof(ev)) == sizeof(ev)
            && ev.type == EV_MSC && ev.code == MSC_SCAN) {
            unsigned code = ev.value;
            long t = ms(&ev.time);
            const struct map *m = lookup(remote, sizeof(remote) / sizeof(remote[0]), code);
            if (code != last_code || t - last_ms > NEW_PRESS_MS) {
                press_ms = t;
                fire("remote", m, code);
            } else {
                fire_held("remote", m, code, t - press_ms);
            }
            last_code = code;
            last_ms = t;
        }
    }
    return NULL;
}

int input_start(void (*on_action)(const char *action))
{
    fd_body = open("/dev/input/event0", O_RDONLY);
    fd_ir = open("/dev/input/event1", O_RDONLY);
    if (fd_body < 0 || fd_ir < 0) {
        printf("input: cannot open event0/event1\n");
        return -1;
    }
    action_cb = on_action;
    run = 1;
    return pthread_create(&thread, NULL, input_thread, NULL);
}

void input_stop(void)
{
    if (!run)
        return;
    run = 0;
    pthread_join(thread, NULL);
    close(fd_body);
    close(fd_ir);
}
