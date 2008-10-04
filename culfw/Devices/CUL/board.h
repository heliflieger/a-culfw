#ifndef _BOARD_H
#define _BOARD_H

#define CC1100_CS_PORT          PORTC
#define CC1100_CS_DDR		DDRC
#define CC1100_CS_PIN		PC5

#define CC1100_INT		INT4
#define CC1100_INTF		INTF4
#define CC1100_INTVECT          INT4_vect
#define CC1100_ISC		ISC40

#define CC1100_PININ            PC7
#define CC1100_PINOUT           PC6
#define CC1100_PINGRP           PINC

#define LED_DDR                 DDRC
#define LED_PORT                PORTC
#define LED_PIN                 PC4
#define BOARD_ID_STR            "CUL"
#define BOARD_ID_USTR           L"CUL"

#define BUSWARE_CUL

#endif
