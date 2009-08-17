#ifndef _CLOCK_H_
#define _CLOCK_H_

#include <MyUSB/Scheduler/Scheduler.h> // Simple scheduler for task management

extern uint8_t day,hour,minute,sec,hsec;
void gettime(char*);

#ifdef HAS_ETHERNET
typedef uint16_t clock_time_t;
clock_time_t clock_time(void);
#endif

TASK(Minute_Task);

#endif
