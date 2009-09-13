#ifndef _BOARD_H
#define _BOARD_H


#ifdef CUN_V10

  #define CC1100_CS_DDR           DDRF
  #define CC1100_CS_PORT          PORTF
  #define CC1100_CS_PIN           PF3
  #define CC1100_IN_DDR           DDRE
  #define CC1100_IN_PORT          PINE
  #define CC1100_IN_PIN           PE6
  #define CC1100_INT              INT6
  #define CC1100_INTVECT          INT6_vect
  #define CC1100_ISC              ISC60
  #define CC1100_EICR             EICRB

  #define ENC28J60_RESET_PORT     PORTD
  #define ENC28J60_RESET_DDR      DDRD
  #define ENC28J60_RESET_BIT      PD6
  #define ENC28J60_SPI_PORT       PORTB
  #define ENC28J60_SPI_DDR        DDRB
  #define ENC28J60_SPI_SCK        PB1
  #define ENC28J60_SPI_MOSI       PB2
  #define ENC28J60_SPI_MISO       PB3
  #define ENC28J60_SPI_SS         PB0
  #define ENC28J60_CONTROL_PORT   PORTD
  #define ENC28J60_CONTROL_DDR    DDRD
  #define ENC28J60_CONTROL_CS     PD7

#else

  #define CC1100_CS_DDR           DDRD
  #define CC1100_CS_PORT          PORTD
  #define CC1100_CS_PIN           PD7
  #define CC1100_IN_DDR           DDRD
  #define CC1100_IN_PORT          PIND
  #define CC1100_IN_PIN           PD2
  #define CC1100_INT              INT2
  #define CC1100_INTVECT          INT2_vect
  #define CC1100_ISC              ISC20
  #define CC1100_EICR             EICRA

  #define ENC28J60_CONTROL_PORT   PORTB
  #define ENC28J60_CONTROL_DDR    DDRB
  #define ENC28J60_CONTROL_CS     PB0
  #define ENC28J60_RESET_PORT     PORTB
  #define ENC28J60_RESET_DDR      DDRB
  #define ENC28J60_RESET_BIT      PB6
  #define ENC28J60_SPI_PORT       SPI_PORT
  #define ENC28J60_SPI_DDR        SPI_DDR
  #define ENC28J60_SPI_SCK        SPI_SCLK
  #define ENC28J60_SPI_MOSI       SPI_MOSI
  #define ENC28J60_SPI_MISO       SPI_MISO
  #define ENC28J60_SPI_SS         SPI_SS

  #define HAS_FS                  1
  #define DF_DDR                  DDRD
  #define DF_PORT                 PORTD 
  #define DF_CS                   PD3

#endif


#define USB_MAX_POWER           250

#define CC1100_OUT_DDR		DDRF
#define CC1100_OUT_PORT         PORTF
#define CC1100_OUT_PIN          PF2

#define LED_DDR                 DDRD
#define LED_PORT                PORTD
#define LED_PIN                 PD1

#define SPI_PORT		PORTB
#define SPI_DDR			DDRB
#define SPI_SS			PB0
#define SPI_MISO		PB3
#define SPI_MOSI		PB2
#define SPI_SCLK		PB1

#define BOARD_ID_STR            "CUN"
#define BOARD_ID_USTR           L"CUN"

#define HAS_USB                 1
#define HAS_ETHERNET            1
#define HAS_XRAM                1
#define HAS_NTP                 1

#undef  HAS_FHT_8v                      // PROGMEM:  434b, RAM: 19b
#define HAS_FHT_80b                     // PROGMEM: 1158b, RAM:  5b
#define FHTBUF_SIZE             74      //                 RAM: 74b

#undef  FULL_CC1100_PA                  // PROGMEM:  100b
#define RCV_BUCKETS             4       //                 RAM: 25b / bucket

#undef  HAS_RAWSEND                     // PROGMEM:   90b  RAM:  6b
#define HAS_FASTRF                      // PROGMEM:  274b  RAM:  4b
#define HAS_LONGMSG                     // CUR support     RAM: 20b

#define BUSWARE_CUN

#endif
