#ifndef _BOARD_H
#define _BOARD_H

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
#define CC1100_EICR             EICRA

#define LED_DDR			DDRD
#define LED_PORT		PORTD
#define LED_PIN			4

#define BOARD_ID_STR		"miniCUL"
#define BOARD_ID_STR433		"miniCUL 433"

#define HAS_UART		1
#define UART_BAUD_RATE		38400

#define TTY_BUFSIZE		174

#define RCV_BUCKETS		4     //                 RAM: 25b * bucket
#define HAS_RF_ROUTER
#define FULL_CC1100_PA
#define HAS_RAWSEND
#define HAS_INTERTECHNO
#define HAS_REVOLT
#define HAS_MEMFN
//#define HAS_OREGON3
#define HAS_CC1101_RX_PLL_LOCK_CHECK_TASK_WAIT
#define HAS_CC1101_PLL_LOCK_CHECK_MSG
#define HAS_CC1101_PLL_LOCK_CHECK_MSG_SW
#define HAS_IT
#define HAS_TCM97001
#define HAS_SOMFY_RTS
#define HAS_HMS

#endif
