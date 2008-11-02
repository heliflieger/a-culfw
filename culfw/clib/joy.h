#ifndef __JOYSTICK
#define __JOYSTICK

#include <avr/io.h>
#include "board.h"
#include <MyUSB/Scheduler/Scheduler.h> // Simple scheduler for task management

void joy_init(void);
extern void (*joyfunc)(uint8_t);
TASK(JOY_Task);

#define KEY_NONE  0
#define KEY_UP    1
#define KEY_DOWN  2
#define KEY_LEFT  3
#define KEY_RIGHT 4
#define KEY_ENTER 5

#endif
