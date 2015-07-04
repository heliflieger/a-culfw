#ifndef _RF_MAICO_H
#define _RF_MAICO_H

#define MAX_MAICO_MSG 30

extern uint8_t maico_on;

void rf_maico_init(void);
void rf_maico_task(void);
void maico_func(char *in);

#endif
