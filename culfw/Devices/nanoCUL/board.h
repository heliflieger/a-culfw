#ifndef _BOARD_H
#define _BOARD_H

#include <stdint.h>

/* if you have an Arduino with only 8MHz disable the next line */
#define HAS_16MHZ_CLOCK

/* if you are using a CC1101 module for 868MHz disable the next line */
#define HAS_CC1100_433


#define SPI_PORT		PORTB
#define SPI_DDR			DDRB
#define SPI_SS			2
#define SPI_MISO		4
#define SPI_MOSI		3
/* die aufgelötete gelbe LED ist an PB5/SCLK angeschlossen! */
#define SPI_SCLK		5

#define CC1100_CS_DDR		SPI_DDR
#define CC1100_CS_PORT          SPI_PORT
#define CC1100_CS_PIN		SPI_SS


/* CC1101 GDO0 Tx / Temperature Sensor */
#if 0
#define CC1100_OUT_DDR		DDRC
#define CC1100_OUT_PORT         PORTC
#define CC1100_OUT_PIN          PC0
#define CC1100_OUT_IN           PINC
#define CCTEMP_MUX              CC1100_OUT_PIN
#else
#define CC1100_OUT_DDR		DDRD
#define CC1100_OUT_PORT         PORTD
#define CC1100_OUT_PIN          PD3
#define CC1100_OUT_IN           PIND
#define CCTEMP_MUX              CC1100_OUT_PIN
#endif

/* CC1101 GDO2 Rx Interrupt */
#define CC1100_IN_DDR		DDRD
#define CC1100_IN_PORT          PIND
#define CC1100_IN_PIN           PD2
#define CC1100_IN_IN            PIND

#define CC1100_INT		INT0
#define CC1100_INTVECT          INT0_vect
#define CC1100_ISC		ISC00
#define CC1100_EICR             EICRA

/* externe LED */
#define LED_DDR                 DDRB
#define LED_PORT                PORTB
#define LED_PIN                 1

//#define LED_ON_DDR              DDRB
//#define LED_ON_PORT             PORTB
//#define LED_ON_PIN              1


#define BOARD_ID_STR            "nanoCUL"
#define BOARD_ID_STR433         "nanoCUL 433"

/* define this device as a 433 MHz one */
/* this isn't done like a CUL by reading a port pin but instead a fixed value of 0 for mark433_pin is used */ 
#define MULTI_FREQ_DEVICE
#define MARK433_PIN 			mark433_pin
#define MARK433_BIT             0
extern const uint8_t mark433_pin;

#define HAS_UART
#define UART_BAUD_RATE          38400

/* ATMega328P has only one UART, no need to define the UART to use */
//#define USART_RX_vect           USART0_RX_vect
//#define USART_UDRE_vect         USART0_UDRE_vect

#define TTY_BUFSIZE             128


#define RCV_BUCKETS            2      //                 RAM: 25b * bucket
#define FULL_CC1100_PA                // PROGMEM:  108b
#define HAS_RAWSEND                   //
#define HAS_FASTRF                    // PROGMEM:  468b  RAM:  1b
#define HAS_ASKSIN
/* Intertechno Senden einschalten */
#define HAS_INTERTECHNO
/* Intertechno Empfang einschalten */
#define HAS_IT
#define HAS_REVOLT
#define HAS_CC1101_RX_PLL_LOCK_CHECK_TASK_WAIT
#define HAS_CC1101_PLL_LOCK_CHECK_MSG
#define HAS_CC1101_PLL_LOCK_CHECK_MSG_SW
/* HAS_MBUS requires about 1kB RAM, if you want to use it you
   should consider disabling other unneeded features
   to avoid stack overflows
*/
//#define HAS_MBUS

#  define HAS_ASKSIN_FUP
#  define HAS_MORITZ
#  define HAS_RWE
#  define HAS_ESA
#  define HAS_TX3
#  define HAS_UNIROLL
#  define HAS_HOERMANN
#  define HAS_MEMFN
#  define HAS_SOMFY_RTS

#endif
