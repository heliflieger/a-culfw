#ifndef _RF_MORITZ_H
#define _RF_MORITZ_H

#include <stdint.h>                     // for uint8_t

#define MAX_MORITZ_MSG 30

extern uint8_t moritz_on;

void rf_moritz_init(void);
void rf_moritz_task(void);
void moritz_func(char *in);

#endif
