#ifndef _RF_BETTY_H
#define _RF_BETTY_H

#define MAX_BETTY_MSG     60

#define BETTY_WOR_TICKS   70

extern uint8_t betty_on;

void rf_betty_init(void);
void rf_betty_task(void);
void betty_func(char *in);

#endif
