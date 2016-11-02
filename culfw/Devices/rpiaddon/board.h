#ifndef _BOARD_H
#define _BOARD_H

#define RPI_ADDON_BOARD

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

#define LED_DDR                 DDRA
#define LED_PORT                PORTA
#define LED_PIN                 1

#define LED_ON_DDR              DDRA
#define LED_ON_PORT             PORTA
#define LED_ON_PIN              0

#define HAS_IRRX
#define HAS_IRTX
#define F_INTERRUPTS            15625   // interrupts per second, min: 10000, max: 20000
#define IRSND_OCx               IRSND_OC2B          // use OC2B/PIN 15

#define BOARD_ID_STR            "RPIAddOn_CSM" // must contain CSM or OWX.pm won't detect it

#define HAS_UART
#define UART_BAUD_RATE          38400
#define USART_RX_vect           USART0_RX_vect
#define USART_UDRE_vect         USART0_UDRE_vect

#define TTY_BUFSIZE             128

#define HAS_FHT_80b                   // PROGMEM: 1374b, RAM: 90b
#define HAS_FHT_8v                    // PROGMEM:  586b  RAM: 23b
#define HAS_FHT_TF
#define HAS_RF_ROUTER                 // PROGMEM: 1248b  RAM: 44b

#define FHTBUF_SIZE          174      //                 RAM: 174b
#define RCV_BUCKETS            4      //                 RAM: 25b * bucket
#define HAS_RF_ROUTER
#define RFR_DEBUG                     // PROGMEM:  354b  RAM: 14b
#define FULL_CC1100_PA                // PROGMEM:  108b
#define HAS_RAWSEND                   //
#define HAS_FASTRF                    // PROGMEM:  468b  RAM:  1b
#define HAS_ASKSIN
#define HAS_ASKSIN_FUP
#define HAS_MORITZ
#define HAS_RWE
#define HAS_ESA
#define HAS_KOPP_FC
#define HAS_TX3
#define HAS_INTERTECHNO
#define HAS_IT
#define HAS_UNIROLL
#define HAS_REVOLT
#define HAS_HOERMANN
#define HAS_MEMFN
#define HAS_CC1101_RX_PLL_LOCK_CHECK_TASK_WAIT
#define HAS_CC1101_PLL_LOCK_CHECK_MSG
#define HAS_CC1101_PLL_LOCK_CHECK_MSG_SW
#define HAS_SOMFY_RTS
#define HAS_TCM97001
#define HAS_HOMEEASY
#define HAS_HMS
#define HAS_BELFOX
#define HAS_RFNATIVE
#define LACROSSE_HMS_EMU
#define HAS_MBUS
#define MBUS_NO_TX                    // MBUS TX eats up lots of memory, OFF by default
#define HAS_ZWAVE

/* a maximum of 8 onewire devices is supported */
#define HAS_ONEWIRE            8      // OneWire Device Buffer, RAM: 10 * 8 Byte

#define RPI_TTY_FIX

#endif
