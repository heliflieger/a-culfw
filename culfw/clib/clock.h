#ifndef _CLOCK_H_
#define _CLOCK_H_

#include <MyUSB/Scheduler/Scheduler.h> // Simple scheduler for task management

extern uint8_t day,hour,minute,sec,hsec;
void gettime(char*);

// Seconds - needed for Ethernet
typedef uint16_t clock_time_t;
#define CLOCK_CONF_SECOND	1
clock_time_t clock_time(void);

TASK(Minute_Task);

#endif
