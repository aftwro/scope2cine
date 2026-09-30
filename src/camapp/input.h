#ifndef CAMAPP_INPUT_H
#define CAMAPP_INPUT_H

/* Reads the body buttons (/dev/input/event0) and the IR remote (/dev/input/event1),
 * maps each press to an action name and hands it to on_action (same syntax as the
 * command FIFO). Returns 0 on success. */
int input_start(void (*on_action)(const char *action));
void input_stop(void);

#endif
