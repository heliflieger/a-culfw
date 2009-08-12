#ifndef _FASTRF_H
#define _FASTRF_H

void fastrf(char *in);
void fastrf_reset(void);
extern uint8_t fastrf_on;

#include <MyUSB/Scheduler/Scheduler.h> // Simple scheduler for task management
TASK(FastRF_Task);

#endif
