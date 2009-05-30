#ifndef _BOARD_H
#define _BOARD_H

#define CC1100_CS_DDR		DDRC
#define CC1100_CS_PORT          PORTC
#define CC1100_CS_PIN		PC5
#define CC1100_IN_DDR		DDRC
#define CC1100_IN_PORT          PINC
#define CC1100_IN_PIN           PC7
#define CC1100_OUT_DDR		DDRC
#define CC1100_OUT_PORT         PORTC
#define CC1100_OUT_PIN          PC6
#define CC1100_INT		INT4
#define CC1100_INTVECT          INT4_vect
#define CC1100_ISC		ISC40

#define LED_DDR                 DDRC
#define LED_PORT                PORTC
#define LED_PIN                 PC4

#define BOARD_ID_STR            "CUL"
#define BOARD_ID_USTR           L"CUL"

#define SPI_PORT		PORTB
#define SPI_DDR			DDRB
#define SPI_SS			PB0
#define SPI_MISO		PB3
#define SPI_MOSI		PB2
#define SPI_SCLK		PB1

#define HAS_USB                 1
#undef  HAS_FHT_8v
#define HAS_FHT_80b

#define BUSWARE_CUL

#endif
