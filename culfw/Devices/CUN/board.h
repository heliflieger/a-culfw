#ifndef _BOARD_H
#define _BOARD_H

#include <avr/io.h>

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

// if you have external FLASH memory ...
//  #define HAS_FS                  1
  #define DF_DDR                  DDRD
  #define DF_PORT                 PORTD 
  #define DF_CS                   PD3

  #define CCTEMP_MUX              2

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

#define HAS_USB                 1       // undef or define...1
#define HAS_ETHERNET            1       // undef or define...1
#define HAS_ETHERNET_KEEPALIVE  1
#define ETHERNET_KEEPALIVE_TIME 30
#define HAS_XRAM                1       // undef or define...1
#define HAS_NTP                 1       // undef or define...1

#define TTY_BUFSIZE             64
#define USB_BUFSIZE             64      // Must be a supported USB endpoint size
#define HAS_FHT_8v                      // PROGMEM:  586b, RAM: 23b
#define HAS_FHT_80b                     // PROGMEM: 1374b, RAM: 90b
#define HAS_FHT_TF
#define FHTBUF_SIZE            200      //                 RAM:200b
#define HAS_RF_ROUTER           1       // PROGMEM: 1106b, RAM: 43b
#define RCV_BUCKETS             4       //                 RAM: 25b / bucket

#define FULL_CC1100_PA                  // PROGMEM:  108b
#define HAS_RAWSEND                     // PROGMEM:  198b     RAM:  4b
#define HAS_FASTRF                      // PROGMEM:  362+106  RAM:  1b
#define HAS_ASKSIN
#define HAS_ESA
#define HAS_TX3
#define HAS_INTERTECHNO
#define HAS_HOERMANN
#define HAS_IT
#define HAS_TCM97001
#define HAS_HOMEEASY
#define HAS_HMS
#define HAS_MANCHESTER
#define HAS_REVOLT

#define HAS_CC1101_RX_PLL_LOCK_CHECK_TASK_WAIT
#define HAS_CC1101_PLL_LOCK_CHECK_MSG
#define HAS_CC1101_PLL_LOCK_CHECK_MSG_SW

#define BUSWARE_CUN

#endif
