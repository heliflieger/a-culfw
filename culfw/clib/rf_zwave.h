#ifndef _RF_ZWAVE_H
#define _RF_ZWAVE_H

#include <stdint.h>                     // for uint8_t

#ifdef HAS_MULTI_CC
#define NUM_ZWAVE    HAS_MULTI_CC
#else
#define NUM_ZWAVE    1
#endif

extern uint8_t zwave_on[NUM_ZWAVE];
void rf_zwave_init(void);
void rf_zwave_task(void);
void zwave_func(char *in);

#endif
