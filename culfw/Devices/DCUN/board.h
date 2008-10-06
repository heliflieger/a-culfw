#ifndef _BOARD_H
#define _BOARD_H

#define CC1100_CS_PORT          PORTD
#define CC1100_CS_DDR		DDRD
#define CC1100_CS_PIN		PD6

#define CC1100_INT		INT5
#define CC1100_INTF		INTF5
#define CC1100_INTVECT          INT5_vect
#define CC1100_ISC		ISC50

#define CC1100_PININ            PE5
#define CC1100_PINOUT           PE3
#define CC1100_PINGRP           PINE

#define LED_DDR                 DDRE
#define LED_PORT                PORTE
#define LED_PIN                 PE0
#define BOARD_ID_STR            "DCUN"
#define BOARD_ID_USTR           L"Dual CC110x USB Network DCUN"

#define BUSWARE_DCUN

#endif
