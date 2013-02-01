#ifndef _RADI0_H
#define _RADIO_H

#include "../../board.h"

void radio_init(void);
void radio_sendchar(uint8_t data);
uint8_t radio_recvchar(void);
uint8_t radio_avail(void);

#endif
