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
#define SPI_IN			PINB
#define SPI_SS			0
#define SPI_MISO		3
#define SPI_MOSI		2
#define SPI_SCLK		1

#define CC1100_CS_DDR		DDRB
#define CC1100_CS_PORT          PORTB
#define CC1100_CS_PIN		4

#define CC1100_OUT_DDR          DDRB
#define CC1100_OUT_PORT         PORTB
#define CC1100_OUT_PIN          5
#define CC1100_OUT_IN           PINB

#define CC1100_IN_DDR	        DDRE
#define CC1100_IN_PORT          PINE
#define CC1100_IN_PIN           6
#define CC1100_IN_IN            PINE
#define CC1100_INT	        INT6
#define CC1100_INTVECT          INT6_vect
#define CC1100_ISC	        ISC60
#define CC1100_EICR             EICRB
#define LED_DDR                 DDRB
#define LED_PORT                PORTB
#define LED_PIN                 0

#define HAS_UART                1
#define UART_BAUD_RATE          38400
#define USART_RX_vect           USART1_RX_vect
#define USART_UDRE_vect         USART1_UDRE_vect

// we map uart 1 => uart 0 here (which is not existing)
#define UDR0 UDR1
#define UCSR0B UCSR1B
#define UDRIE0 UDRIE1
#define UCSR0A UCSR1A
#define FE0 FE1
#define DOR0 DOR1
#define U2X0 U2X1
#define UBRR0 UBRR1
#define RXCIE0 RXCIE1
#define RXEN0 RXEN1
#define TXEN0 TXEN1
#define UCSR0C UCSR1C
#define UCSZ00 UCSZ10
#define UCSZ01 UCSZ11
#define UDRIE0 UDRIE1

#define FHTBUF_SIZE          174      //                 RAM: 174b
#define RCV_BUCKETS            4      //                 RAM: 25b * bucket
#define RFR_DEBUG                     // PROGMEM:  354b  RAM: 14b
#define FULL_CC1100_PA                // PROGMEM:  108b
#define HAS_RAWSEND                   //
#define HAS_FASTRF                    // PROGMEM:  468b  RAM:  1b
#define HAS_RF_ROUTER                 // PROGMEM:  920b  RAM: 38b
#define HAS_ASKSIN
#define HAS_MORITZ
#define HAS_ESA
#define HAS_INTERTECHNO
#define HAS_TCM97001
#define HAS_SOMFY_RTS
#define HAS_MBUS
#define HAS_CC1101_RX_PLL_LOCK_CHECK_TASK_WAIT
#define HAS_CC1101_PLL_LOCK_CHECK_MSG
#define HAS_CC1101_PLL_LOCK_CHECK_MSG_SW
#define HAS_ASKSIN_FUP
#define HAS_RWE
#define HAS_TX3
#define HAS_UNIROLL
#define HAS_HOERMANN
#define HAS_IT
#define HAS_HOMEEASY
#define HAS_HMS

#define MULTI_FREQ_DEVICE       // available in multiple versions: 433MHz,868MHz,915MHz

#define MARK433_PORT            PORTD
#define MARK433_PIN             PIND
#define MARK433_BIT             4

#define BOARD_ID_STR            "CSM868"
#define BOARD_ID_STR433         "CSM433"
#define BOARD_ID_STR915         "CSM915"

#define HAS_UART                1
#define UART_BAUD_RATE          38400

#define TTY_BUFSIZE             128

#define BUSWARE_CSM

#endif
