#ifndef _BOARD_H
#define _BOARD_H

#include <avr/io.h>

#define HAS_FHT_8v			// PROGMEM:  434b, RAM: 19b
#define HAS_FHT_80b			// PROGMEM: 1158b, RAM:  5b
#define FHTBUF_SIZE		174	//                 RAM: 174b
#define RCV_BUCKETS		4	//                 RAM: 25b * bucket
#undef  RFR_DEBUG			// PROGMEM:  354b  RAM: 14b
#define FULL_CC1100_PA			// PROGMEM:  108b
#define HAS_RAWSEND			//
#define HAS_FASTRF			// PROGMEM:  468b  RAM:  1b
#define HAS_ASKSIN
#define HAS_ASKSIN_FUP
#undef  HAS_ESA
#undef  HAS_INTERTECHNO
#define HAS_TCM97001
#define HAS_MORITZ
#undef  HAS_RWE
#define HAS_TX3
#define HAS_HOERMANN
#undef  HAS_MEMFN
#define HAS_RF_ROUTER
#define HAS_CC1101_RX_PLL_LOCK_CHECK_TASK_WAIT
#define HAS_CC1101_PLL_LOCK_CHECK_MSG
#define HAS_CC1101_PLL_LOCK_CHECK_MSG_SW
#define HAS_IT
#define HAS_HMS

/*
 * Board definition according to
 * http://www.seeedstudio.com/depot/datasheet/RFBee%20User%20Manual%20v1.1.pdf
 */

#define SPI_PORT		PORTB
#define SPI_DDR			DDRB
#define SPI_SS			2
#define SPI_MISO		4
#define SPI_MOSI		3
#define SPI_SCLK		5

#define CC1100_CS_DDR		SPI_DDR
#define CC1100_CS_PORT		SPI_PORT
#define CC1100_CS_PIN		SPI_SS

#define CC1100_OUT_DDR		DDRD
#define CC1100_OUT_PORT		PORTD
#define CC1100_OUT_PIN		2

#define CC1100_IN_DDR		DDRD
#define CC1100_IN_PORT		PIND
#define CC1100_IN_PIN		3
#define CC1100_INT		INT1
#define CC1100_INTVECT		INT1_vect
#define CC1100_ISC		ISC10
#define CC1100_EICR		EICRA

#define LED_DDR			DDRD
#define LED_PORT		PORTD
#define LED_PIN			6

#define BOARD_ID_STR		"RFbee"

#define HAS_UART		1
#define UART_BAUD_RATE		38400

#define TTY_BUFSIZE		104

#endif
