#ifndef _BOARD_H
#define _BOARD_H

// Feature definitions
#define BOARD_ID_STR            "CUL-ARDUINO868"
#define BOARD_ID_USTR           L"CUL-ARDUINO868"

#define MULTI_FREQ_DEVICE	// available in multiple versions: 433MHz,868MHz
#define BOARD_ID_STR433         "CUL-ARDUINO433"
#define BOARD_ID_USTR433        L"CUL-ARDUINO433"

#define HAS_USB                  1
#define USB_BUFSIZE             64      // Must be a supported USB endpoint size
#define USB_MAX_POWER	       100
#define HAS_FHT_80b                     // PROGMEM: 1374b, RAM: 90b
#define HAS_RF_ROUTER                   // PROGMEM: 1248b  RAM: 44b
#define HAS_CC1101_RX_PLL_LOCK_CHECK_TASK_WAIT	// PROGMEM: 118b
#define HAS_CC1101_PLL_LOCK_CHECK_MSG		// PROGMEM:  22b
#define HAS_CC1101_PLL_LOCK_CHECK_MSG_SW	// PROGMEM:  22b

#if defined(CUL_V3_ZWAVE)
#  define CUL_V3
#endif

#if defined(CUL_ARDUINO)
#  define HAS_FHT_8v                    // PROGMEM:  586b  RAM: 23b
#  define HAS_FHT_TF
#  define FHTBUF_SIZE          174      //                 RAM: 174b
#  define RCV_BUCKETS            4      //                 RAM: 25b * bucket
#  define RFR_DEBUG                     // PROGMEM:  354b  RAM: 14b
#  define FULL_CC1100_PA                // PROGMEM:  108b
#  define HAS_RAWSEND                   //
#  define HAS_FASTRF                    // PROGMEM:  468b  RAM:  1b
#  define HAS_ASKSIN
#  define HAS_ASKSIN_FUP
#  define HAS_KOPP_FC
#  define HAS_RWE
#  define HAS_TX3
#  define HAS_INTERTECHNO
#  define HAS_UNIROLL
#  define HAS_MEMFN
#  define HAS_SOMFY_RTS
#  define HAS_RFNATIVE

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
#  define HAS_MBUS                      // PROGMEM: 2536
#  define MBUS_NO_TX                       // PROGMEM:  962
#  define HAS_RFNATIVE                  // PROGMEM:  580
//#  define LACROSSE_HMS_EMU              // PROGMEM: 2206
#  define HAS_KOPP_FC                   // PROGMEM: 3370
#endif


#endif


#if defined(CUL_ARDUINO)
#  define TTY_BUFSIZE          128      // RAM: TTY_BUFSIZE*4
#endif




// No features to define below

#include <avr/io.h>
#include <avr/power.h>

#if !defined(clock_prescale_set) && __AVR_LIBC_VERSION__  < 10701UL
#  warning "avr/power.h needs patching for prescaler functions to work."
#  warning "for the m32u4 add __AVR_ATmega32U4__ for cpu types on prescale block"
#  warning "for the m32u2 add __AVR_ATmega32U2__ for cpu types on prescale block"
#endif

#if defined(CUL_ARDUINO)      // not sure why libc is missing those ...
#  define PB0 PORTB0
#  define PB1 PORTB1
#  define PB2 PORTB2
#  define PB3 PORTB3
#  define PB6 PORTB6
#  define PD2 PORTD2
#  define PD3 PORTD3
#endif  // CUL_ARDUINO

#define SPI_PORT		PORTB
#define SPI_DDR			DDRB
#define SPI_SS			PB0
#define SPI_MISO		PB3
#define SPI_MOSI		PB2
#define SPI_SCLK		PB1


#if defined(CUL_ARDUINO)
#  define CC1100_CS_DDR		SPI_DDR
#  define CC1100_CS_PORT        SPI_PORT
#  define CC1100_CS_PIN		SPI_SS
#  define CC1100_OUT_DDR        DDRD
#  define CC1100_OUT_PORT       PORTD
#  define CC1100_OUT_PIN        PD3
#  define CC1100_OUT_IN         PIND
#  define CC1100_IN_DDR		DDRD
#  define CC1100_IN_PORT        PIND
#  define CC1100_IN_PIN         PD2
#  define CC1100_IN_IN          PIND
#  define CC1100_INT		INT2
#  define CC1100_INTVECT        INT2_vect
#  define CC1100_ISC		ISC20
#  define CC1100_EICR           EICRA
#endif

// Ports for LED on Arduino RPro Micro Board
#if defined(CUL_ARDUINO)
#  define LED_DDR               DDRD
#  define LED_PORT              PORTD
#  define LED_PIN               5
#  define LED_INV
#endif


#if defined(CUL_ARDUINO) 
#  define CUL_HW_REVISION "CUL-ARDUINO"
#else
//#  define CUL_HW_REVISION "CUL_V2"    // No more mem for this feature
#endif

#define MARK433_PORT            PORTB
#define MARK433_PIN             PINB
#define MARK433_BIT             6
#define MARK915_PORT            PORTB
#define MARK915_PIN             PINB
#define MARK915_BIT             5

#endif // __BOARD_H__

