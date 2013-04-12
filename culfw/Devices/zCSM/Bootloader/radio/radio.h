#ifndef _RADI0_H
#define _RADIO_H

#include "../../board.h"

#define F_CPU 8000000UL

void radio_init(void);
void radio_sendchar(uint8_t data);
uint8_t radio_recvchar(void);
uint8_t radio_avail(void);



#endif
