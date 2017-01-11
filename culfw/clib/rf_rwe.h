#ifndef _RF_RWE_H
#define _RF_RWE_H

#include <stdint.h>                     // for uint8_t

#define MAX_RWE_MSG 30

extern uint8_t rwe_on;

void rf_rwe_init(void);
void rf_rwe_task(void);
void rwe_func(char *in);

#endif
