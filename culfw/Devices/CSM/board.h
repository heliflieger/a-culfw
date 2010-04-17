#ifndef _BOARD_H
#define _BOARD_H

#define CSMV3

#ifdef CSMV3
#define CC1100_CS_DDR		DDRD
#define CC1100_CS_PORT          PORTD
#define CC1100_CS_PIN		5

#define CC1100_OUT_DDR		DDRC
#define CC1100_OUT_PORT         PORTC
#define CC1100_OUT_PIN          6

#define CC1100_IN_DDR		DDRD
#define CC1100_IN_PORT          PIND
#define CC1100_IN_PIN           3
#define CC1100_INT		INT1
#define CC1100_INTVECT          INT1_vect
#define CC1100_ISC		ISC10
#define CC1100_EICR             EICRA

#define LED_DDR                 DDRD
#define LED_PORT                PORTD
#define LED_PIN                 6
#endif

#ifdef CSMV2
#define CC1100_CS_DDR		DDRD
#define CC1100_CS_PORT          PORTD
#define CC1100_CS_PIN		6

#define CC1100_OUT_DDR		DDRD
#define CC1100_OUT_PORT         PORTD
#define CC1100_OUT_PIN          5

#define CC1100_IN_DDR		DDRD
#define CC1100_IN_PORT          PIND
#define CC1100_IN_PIN           3
#define CC1100_INT		INT1
#define CC1100_INTVECT          INT1_vect
#define CC1100_ISC		ISC10
#define CC1100_EICR             EICRA

#define LED_DDR                 DDRD
#define LED_PORT                PORTD
#define LED_PIN                 2
#endif 

#define BOARD_ID_STR            "CSM868"
#define BOARD_ID_STR433         "CSM433"

#define SPI_PORT		PORTB
#define SPI_DDR			DDRB
#define SPI_SS			2
#define SPI_MISO		4
#define SPI_MOSI		3
#define SPI_SCLK		5

#define HAS_UART                1
#define UART_BAUD_RATE          9600      

#define TTY_BUFSIZE             32
#define USB_BUFSIZE             32
#define HAS_FHT_8v                      // PROGMEM:  434b, RAM: 19b
#undef HAS_FHT_80b                     // PROGMEM: 1158b, RAM:  5b
#define FHTBUF_SIZE             74      //                 RAM: 74b

#undef  FULL_CC1100_PA                  // PROGMEM:  108b
#define RCV_BUCKETS             2       //                 RAM: 25b / bucket

#undef  HAS_RAWSEND                     // PROGMEM:  198b     RAM:  4b
#undef  HAS_FASTRF                      // PROGMEM:  362+106  RAM:  1b
#undef  HAS_RF_ROUTER                   // PROGMEM:  920b  RAM: 38b
#undef  HAS_LONGMSG                     // CUR support     RAM: 20b


#define BUSWARE_CSM

#endif
