#ifndef _BOARD_H
#define _BOARD_H

#define BOARD_ID_STR            "CUR"
#define BOARD_ID_USTR           L"CUR"

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
#define LED_INV

#define HAS_USB
#define HAS_GLCD
#define USB_OPTIONAL

#define LCD_BL_DDR              DDRC
#define LCD_BL_PORT             PORTC
#define LCD_BL_PIN              PC7

#define LCD_PORT PORTB
#define LCD_DIR  DDRB
#define LCD_CS   PB4
#define LCD_RST  PE3

#define BAT_PORT                PORTF
#define BAT_PIN                 PINF
#define BAT_PIN1                PF1
#define BAT_PIN2                PF2
#define BAT_MUX                 3

#define BUSWARE_CUR


#endif
