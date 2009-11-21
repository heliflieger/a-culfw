#ifndef _BOARD_H
#define _BOARD_H

#define CC1100_CS_DDR		DDRC
#define CC1100_CS_PORT          PORTC
#define CC1100_CS_PIN		PC5
#define CC1100_IN_DDR		DDRC
#define CC1100_IN_PORT          PINC
#define CC1100_IN_PIN           PC7
#define CC1100_OUT_DDR		DDRC
#define CC1100_OUT_PORT         PORTC
#define CC1100_OUT_PIN          PC6
#define CC1100_INT		INT4
#define CC1100_INTVECT          INT4_vect
#define CC1100_ISC		ISC40
#define CC1100_EICR             EICRB

#define USB_MAX_POWER		100

#define LED_DDR                 DDRC
#define LED_PORT                PORTC
#define LED_PIN                 PC4

#define BOARD_ID_STR            "CUL868"
#define BOARD_ID_STR433         "CUL433"
#define BOARD_ID_USTR           L"CUL868"
#define BOARD_ID_USTR433        L"CUL433"

#define SPI_PORT		PORTB
#define SPI_DDR			DDRB
#define SPI_SS			PB0
#define SPI_MISO		PB3
#define SPI_MOSI		PB2
#define SPI_SCLK		PB1

#define HAS_USB                 1

#define TTY_BUFSIZE             32      // Must be a supported USB endpoint size
#define HAS_FHT_8v                      // PROGMEM:  586b, RAM: 23b
#define HAS_FHT_80b                     // PROGMEM: 1374b, RAM: 90b
#define FHTBUF_SIZE             74      //                 RAM: 74b
#define HAS_RF_ROUTER           1       // PROGMEM: 1106b, RAM: 43b
#define RCV_BUCKETS             2       //                 RAM: 25b / bucket

//#define  FULL_CC1100_PA                 // PROGMEM:  108b
//#define  HAS_RAWSEND                    // PROGMEM:  198b     RAM:  4b
//#define  HAS_FASTRF                     // PROGMEM:  362+106  RAM:  1b

#define BUSWARE_CUL

#endif
