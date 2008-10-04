#ifndef _CLOCK_H_
#define _CLOCK_H_

#include <MyUSB/Scheduler/Scheduler.h> // Simple scheduler for task management

extern uint8_t day,hour,minute,sec,hsec;
void gettime(char*);

TASK(Minute_Task);

#endif
