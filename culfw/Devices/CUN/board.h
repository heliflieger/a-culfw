#ifndef _BOARD_H
#define _BOARD_H

#define CC1100_CS_DDR		DDRF
#define CC1100_CS_PORT          PORTF
#define CC1100_CS_PIN		PF3
#define CC1100_IN_DDR		DDRE
#define CC1100_IN_PORT          PINE
#define CC1100_IN_PIN           PE6
#define CC1100_OUT_DDR		DDRF
#define CC1100_OUT_PORT         PORTF
#define CC1100_OUT_PIN          PF2
#define CC1100_INT		INT6
#define CC1100_INTVECT          INT6_vect
#define CC1100_ISC		ISC60

#define LED_DDR                 DDRD
#define LED_PORT                PORTD
#define LED_PIN                 PD1

#define BOARD_ID_STR            "CUN"
#define BOARD_ID_USTR           L"CUN"

#define SPI_PORT		PORTB
#define SPI_DDR			DDRB
#define SPI_SS			PB0
#define SPI_MISO		PB3
#define SPI_MOSI		PB2
#define SPI_SCLK		PB1

#define HAS_USB                 1
#define HAS_ETHERNET            1
#define HAS_XRAM                1
#define no_HAS_TOSC             1       // timer2 is clocked externally
#define no_DEMOMODE             1

#undef  HAS_FHT_8v                      // PROGMEM:  434b, RAM: 19b
#define HAS_FHT_80b                     // PROGMEM: 1158b, RAM:  5b
#define FHTBUF_SIZE             74      //                 RAM: 74b

#undef  FULL_CC1100_PA                  // PROGMEM:  100b
#define RCV_BUCKETS             2       //                 RAM: 25b / bucket

#undef  HAS_RAWSEND                     // PROGMEM:   90b  RAM:  6b
#define HAS_FASTRF                      // PROGMEM:  274b  RAM:  4b
#define HAS_LONGMSG                     // CUR support     RAM: 20b


#define BUSWARE_CUN

#endif
