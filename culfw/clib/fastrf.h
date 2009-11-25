#ifndef _FASTRF_H
#define _FASTRF_H

#define FASTRF_MODE_ON	1
#define FASTRF_MODE_OFF	0

void fastrf_mode(uint8_t on);
void fastrf_func(char *in);
extern uint8_t fastrf_on;

void FastRF_Task(void);

#endif
