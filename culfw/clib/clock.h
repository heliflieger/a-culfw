#ifndef _CLOCK_H_
#define _CLOCK_H_

extern uint8_t day,hour,minute,sec,hsec;
void gettime(char*);

#ifdef HAS_ETHERNET

typedef uint16_t clock_time_t;
clock_time_t clock_time(void);

#endif

void Minute_Task(void);

#endif
