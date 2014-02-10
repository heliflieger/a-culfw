#ifndef _RF_ASKSIN_H
#define _RF_ASKSIN_H

#ifndef HAS_ASKSIN_FUP
#define MAX_ASKSIN_MSG 30
#else
#define MAX_ASKSIN_MSG 64
#endif

extern uint8_t asksin_on;

void rf_asksin_init(void);
void rf_asksin_task(void);
void asksin_func(char *in);

#endif
