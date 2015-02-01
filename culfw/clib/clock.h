#ifndef _CLOCK_H_
#define _CLOCK_H_

volatile extern uint32_t ticks;  // 1/125 sec resolution
void gettime(char*);
void Minute_Task(void);
void get_timestamp(uint32_t *ts);

#ifdef HAS_ETHERNET
typedef uint16_t clock_time_t;
clock_time_t clock_time(void);
#endif

#endif
