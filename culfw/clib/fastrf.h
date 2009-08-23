#ifndef _FASTRF_H
#define _FASTRF_H

void fastrf(char *in);
void fastrf_reset(void);
extern uint8_t fastrf_on;

void FastRF_Task(void);

#endif
