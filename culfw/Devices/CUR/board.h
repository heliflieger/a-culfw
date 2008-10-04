#ifndef _BOARD_H
#define _BOARD_H

#define CC1100_CS_PORT  	PORTE
#define CC1100_CS_DDR		DDRE
#define CC1100_CS_PIN		PE1

#define CC1100_INT		INT5
#define CC1100_INTF		INTF5
#define CC1100_INTVECT  	INT5_vect
#define CC1100_ISC		ISC50

#define CC1100_PININ		PE5
#define CC1100_PINOUT           PE0
#define CC1100_PINGRP		PINE

#define LED_DDR                 DDRC
#define LED_PORT                PORTC
#define LED_PIN                 PC0
#define BOARD_ID_STR            "CUR"
#define BOARD_ID_USTR           L"CUR"

#define BUSWARE_CUR


#endif
