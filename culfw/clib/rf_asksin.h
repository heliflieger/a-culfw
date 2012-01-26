#ifndef _RF_ASKSIN_H
#define _RF_ASKSIN_H

#define MAX_ASKSIN_MSG 30

extern uint8_t asksin_on;

void rf_asksin_init(void);
void rf_asksin_task(void);
void asksin_func(char *in);

#endif
