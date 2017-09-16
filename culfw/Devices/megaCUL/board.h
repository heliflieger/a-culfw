#ifndef _BOARD_H
#define _BOARD_H

#include <stdint.h>

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
#define CC1100_OUT_IN           PINB

#define CC1100_IN_DDR		DDRD
#define CC1100_IN_PORT          PIND
#define CC1100_IN_PIN           2
#define CC1100_IN_IN            PIND

#define CC1100_INT		INT0
#define CC1100_INTVECT          INT0_vect
#define CC1100_ISC		ISC00
#define CC1100_EICR             EICRA

#define LED_DDR                 DDRD
#define LED_PORT                PORTD
#define LED_PIN                 7

#define HAS_IRRX			//IR Receiption
#define F_INTERRUPTS            15625	// interrupts per second, min: 10000, max: 20000
#define IRMP_PORT               PORTB
#define IRMP_DDR                DDRB
#define IRMP_PIN                PINB
#define IRMP_BIT                0       // use PB0 as IR input on AVR

#define HAS_IRTX			//IR Transmission
#define IRSND_OCx               IRSND_OC2B	// use OC2B/PIN 15

#define MULTI_FREQ_DEVICE	// available in multiple versions: 433MHz, 868MHz
#define BOARD_ID_STR            "megaCUL"
#define BOARD_ID_STR433         "megaCUL433"

#define HAS_UART
#define UART_BAUD_RATE          38400
#define USART_RX_vect           USART0_RX_vect
#define USART_UDRE_vect         USART0_UDRE_vect

#define TTY_BUFSIZE             1024            // 4 buffers < 16k SRAM

#define HAS_FHT_80b                 // PROGMEM: 1374b, RAM: 90b
#define HAS_FHT_8v                  // PROGMEM:  586b  RAM: 23b
#define HAS_FHT_TF
#define FHTBUF_SIZE             174 //                 RAM: 174b
#define RCV_BUCKETS               4 //                 RAM: 25b * bucket
#define RFR_DEBUG                   // PROGMEM:  354b  RAM: 14b
#define HAS_RF_ROUTER               // PROGMEM: 1248b  RAM: 44b
#define RFR_FILTER                  // PROGMEM:   90b  RAM:  4b
#define FULL_CC1100_PA              // PROGMEM:  108b
#define HAS_RAWSEND
#define HAS_FASTRF                  // PROGMEM:  468b  RAM:  1b
#define HAS_ASKSIN                  // PROGMEM: 1314
#define HAS_ASKSIN_FUP              // PROGMEM:   78
#define HAS_RWE
#define HAS_TX3                     // PROGMEM:  168
#define HAS_INTERTECHNO             // PROGMEM: 1352
#define HAS_UNIROLL                 // PROGMEM:   92
#define HAS_SOMFY_RTS               // PROGMEM: 1716
#define HAS_MEMFN                   // PROGMEM:  168
#define HAS_CC1101_RX_PLL_LOCK_CHECK_TASK_WAIT  // PROGMEM: 118b
#define HAS_CC1101_PLL_LOCK_CHECK_MSG           // PROGMEM:  22b
#define HAS_CC1101_PLL_LOCK_CHECK_MSG_SW        // PROGMEM:  22b
#define RPI_TTY_FIX

#if defined(_433MHZ)
#define HAS_TCM97001                // PROGMEM:  264
#define HAS_REVOLT
#define HAS_IT
#define HAS_HOMEEASY
#define HAS_MANCHESTER
#define HAS_BELFOX                  // PROGMEM:  214
#endif

#if defined(_868MHZ)
#define HAS_HMS
#define HAS_ESA                     // PROGMEM:  286
#define HAS_MORITZ                  // PROGMEM: 1696
#define HAS_HOERMANN
#define HAS_HOERMANN_SEND           // PROGMEM:  220
#define HAS_RFNATIVE                // PROGMEM:  580
#define LACROSSE_HMS_EMU            // PROGMEM: 2206
#define HAS_MBUS                    // PROGMEM: 2536
#define MBUS_NO_TX                  // PROGMEM:  962
#define HAS_KOPP_FC                 // PROGMEM: 3370
#define HAS_ZWAVE                   // PROGMEM:  882
//#define HAS_EVOHOME
#endif

#define MARK433_PORT            PORTA
#define MARK433_PIN             PINA
#define MARK433_BIT             0
#define MARK915_PORT            PORTA
#define MARK915_PIN             PINA
#define MARK915_BIT             1

#endif