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
#define CC1100_OUT_IN 		PIND

#define CC1100_IN_DDR		DDRD
#define CC1100_IN_PORT		PIND
#define CC1100_IN_PIN		3
#define CC1100_IN_IN 		PIND

#define CC1100_INT		INT1
#define CC1100_INTVECT		INT1_vect
#define CC1100_ISC		ISC10
#define CC1100_EICR             EICRA

#define LED_DDR			DDRD
#define LED_PORT		PORTD
#define LED_PIN			4

#define BOARD_ID_STR		"miniCUL"
#define BOARD_ID_STR433		"miniCUL433"

#define HAS_UART		1
#define UART_BAUD_RATE		38400

#define TTY_BUFSIZE		128

#define HAS_FHT_80b 		// PROGMEM: 1374b, RAM: 90b
#define HAS_FHT_8v 		// PROGMEM:  586b  RAM: 23b
#define HAS_FHT_TF
#define FHTBUF_SIZE 		174 //             RAM: 174b
#define RCV_BUCKETS		4     //                 RAM: 25b * bucket
#define RFR_DEBUG 		// PROGMEM:  354b  RAM: 14b
#define HAS_RF_ROUTER
#define FULL_CC1100_PA
#define HAS_RAWSEND
#define HAS_FASTRF 		// PROGMEM:  468b  RAM:  1b
#define HAS_ASKSIN
#define HAS_ASKSIN_FUP
#define HAS_RWE
#define HAS_TX3
#define HAS_INTERTECHNO
#define HAS_UNIROLL
#define HAS_MEMFN
#define HAS_SOMFY_RTS
#define LACROSSE_HMS_EMU
#define HAS_CC1101_RX_PLL_LOCK_CHECK_TASK_WAIT
#define HAS_CC1101_PLL_LOCK_CHECK_MSG
#define HAS_CC1101_PLL_LOCK_CHECK_MSG_SW


#  if defined(_433MHZ)
#    define HAS_TCM97001
#    define HAS_REVOLT
#    define HAS_IT
#    define HAS_HOMEEASY
#    define HAS_MANCHESTER
#    define HAS_BELFOX
#  endif

#if defined(_868MHZ)
//#    define HAS_HMS
#    define HAS_ESA
#    define HAS_MORITZ
//#    define HAS_HOERMANN
//#  define HAS_MBUS                      // PROGMEM: 2536
#  define MBUS_NO_TX                       // PROGMEM:  962
#  define HAS_RFNATIVE                  // PROGMEM:  580
//#  define LACROSSE_HMS_EMU              // PROGMEM: 2206
#  define HAS_KOPP_FC                   // PROGMEM: 3370
#endif


#define MARK433_PORT            PORTC
#define MARK433_PIN             PINC
#define MARK433_BIT             0
#define MARK915_PORT            PORTC
#define MARK915_PIN             PINC
#define MARK915_BIT             1


#endif
