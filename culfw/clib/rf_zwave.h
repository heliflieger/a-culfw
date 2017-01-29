#ifndef _RF_ZWAVE_H
#define _RF_ZWAVE_H

#include <stdint.h>                     // for uint8_t

extern uint8_t zwave_on;
void rf_zwave_init(void);
void rf_zwave_task(void);
void zwave_func(char *in);

#endif
