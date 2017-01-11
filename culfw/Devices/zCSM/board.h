#ifndef _BOARD_H
#define _BOARD_H

#include <avr/io.h>

#define HAS_FHT_8v                      // PROGMEM:  434b, RAM: 19b
#define HAS_FHT_80b                     // PROGMEM: 1158b, RAM:  5b
#define HAS_FHT_TF

#undef  FULL_CC1100_PA                  // PROGMEM:  108b

#undef  HAS_RAWSEND                     // PROGMEM:  198b     RAM:  4b
#undef  HAS_FASTRF                      // PROGMEM:  362+106  RAM:  1b
#undef  HAS_RF_ROUTER                   // PROGMEM:  920b  RAM: 38b
#undef  HAS_LONGMSG                     // CUR support     RAM: 20b

#define SPI_PORT		PORTB
#define SPI_DDR			DDRB
#define SPI_SS			4
#define SPI_MISO		6
#define SPI_MOSI		5
#define SPI_SCLK		7

#define CC1100_CS_DDR		SPI_DDR
#define CC1100_CS_PORT          SPI_PORT
#define CC1100_CS_PIN		SPI_SS

#define CC1100_OUT_DDR		DDRB
#define CC1100_OUT_PORT         PORTB
#define CC1100_OUT_PIN          1

#define CC1100_IN_DDR		DDRD
#define CC1100_IN_PORT          PIND
#define CC1100_IN_PIN           2
#define CC1100_INT		INT0
#define CC1100_INTVECT          INT0_vect
#define CC1100_ISC		ISC00
#define CC1100_EICR             EICRA

#define LED_DDR                 DDRA
#define LED_PORT                PORTA
#define LED_PIN                 1
#define LED_ON_DDR              DDRA
#define LED_ON_PORT             PORTA
#define LED_ON_PIN              0

#define USART_RX_vect           USART0_RX_vect
#define USART_UDRE_vect         USART0_UDRE_vect

#define FHTBUF_SIZE          174      //                 RAM: 174b
#define RCV_BUCKETS            4      //                 RAM: 25b * bucket
#define RFR_DEBUG                     // PROGMEM:  354b  RAM: 14b
#define FULL_CC1100_PA                // PROGMEM:  108b
#define HAS_RAWSEND                   //
#define HAS_FASTRF                    // PROGMEM:  468b  RAM:  1b
#define HAS_ASKSIN
#define HAS_MORITZ
#define HAS_ESA
#define HAS_INTERTECHNO
#define HAS_CC1101_RX_PLL_LOCK_CHECK_TASK_WAIT
#define HAS_CC1101_PLL_LOCK_CHECK_MSG
#define HAS_CC1101_PLL_LOCK_CHECK_MSG_SW
#define HAS_IT
#define HAS_TCM97001
#define HAS_HMS
#define HAS_HOMEEASY
#define HAS_MANCHESTER
#define HAS_REVOLT

#define BOARD_ID_STR            "CSM868"
#define BOARD_ID_STR433         "CSM433"

#define HAS_UART                1
#define UART_BAUD_RATE          38400

#define TTY_BUFSIZE             128

#define BUSWARE_CSM

#endif
