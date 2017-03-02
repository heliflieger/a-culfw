#ifndef _RF_ASKSIN_H
#define _RF_ASKSIN_H

#include <stdint.h>                     // for uint8_t

#include "board.h"                      // for HAS_ASKSIN_FUP

#define ASKSIN_WAIT_TICKS_CCA	188	//125 Hz

#ifndef HAS_ASKSIN_FUP
#define MAX_ASKSIN_MSG 30
#else
#define MAX_ASKSIN_MSG 50
#endif

#ifndef USE_RF_MODE
extern uint8_t asksin_on;
#endif

void rf_asksin_init(void);
void rf_asksin_task(void);
void asksin_func(char *in);

#endif
