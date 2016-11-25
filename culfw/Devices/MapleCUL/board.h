#ifndef BOARD_H 
#define BOARD_H

#define _BV(a) (1<<(a))
#define bit_is_set(sfr, bit) ((sfr) & _BV(bit))

#define LONG_PULSE

#define TTY_BUFSIZE          128      // RAM: TTY_BUFSIZE*4

#ifdef MapleCUN
#define BOARD_NAME          "MapleCUN"
#define BOARD_ID_STR        "MapleCUN"
#elif defined MapleCUL
#define BOARD_NAME          "MapleCUL"
#define BOARD_ID_STR        "MapleCUL"
#else
#define MapleCUL
#define BOARD_NAME          "MapleCUL"
#define BOARD_ID_STR        "MapleCUL"
#endif


#define ARM

#define HAS_USB
#define USB_IsConnected		(USBD_GetState() == USBD_STATE_CONFIGURED)
//#define USB_DESCRIPTOR_SN	'1'
#define HAS_XRAM
#define UART_BAUD_RATE          115200
//#define HAS_UART                1

#define HAS_FHT_80b
#define HAS_FHT_8v
//#define HAS_RF_ROUTER
#define HAS_CC1101_RX_PLL_LOCK_CHECK_TASK_WAIT
#define HAS_CC1101_PLL_LOCK_CHECK_MSG
#define HAS_CC1101_PLL_LOCK_CHECK_MSG_SW

#define FHTBUF_SIZE          174
#define RCV_BUCKETS            4      //                 RAM: 25b * bucket
//#define RFR_DEBUG
#define FULL_CC1100_PA
#define HAS_RAWSEND
#define HAS_FASTRF
#define HAS_ASKSIN
#define HAS_ASKSIN_FUP
#define HAS_KOPP_FC
#define HAS_MORITZ
#define HAS_RWE
#define HAS_ESA
#define HAS_HMS
#define HAS_TX3
#define HAS_INTERTECHNO
#define HAS_UNIROLL
#define HAS_HOERMANN
#define HAS_SOMFY_RTS
#define HAS_MAICO
#define HAS_RFNATIVE
#define HAS_ZWAVE

#define _433MHZ

#  if defined(_433MHZ)
#    define HAS_TCM97001
#    define HAS_IT
#    define HAS_HOMEEASY
#    define HAS_BELFOX
#    define HAS_MANCHESTER
#    define HAS_REVOLT
#  endif

#define HAS_MBUS
//#define HAS_MEMFN

//additional CC1101 Transceiver
//#define CC1100_ASKSIN		1
//#define CC1100_MORITZ		2
//#define CC1100_MAICO		3

//Internal Transceiver
#define CC1100_0_CS_PIN		  4
#define CC1100_0_CS_GPIO	  GPIOA
#define CC1100_0_OUT_PIN    1
#define CC1100_0_OUT_GPIO   GPIOA
#define CC1100_0_OUT_PORT   GPIOA->IDR
#define CC1100_0_IN_PIN     0
#define CC1100_0_IN_GPIO    GPIOA
#define CC1100_0_IN_PORT    GPIOA->IDR

//External Transceivers
//PORT 1
//#define CC1100_1_CS_PIN		  6
//#define CC1100_1_CS_BASE	  AT91C_BASE_PIOA
//#define CC1100_1_OUT_PIN    28
//#define CC1100_1_OUT_BASE   AT91C_BASE_PIOB
//#define CC1100_1_OUT_PORT   AT91C_BASE_PIOB->PIO_PDSR
//#define CC1100_1_IN_PIN     5
//#define CC1100_1_IN_BASE	  AT91C_BASE_PIOA
//#define CC1100_1_IN_PORT    AT91C_BASE_PIOA->PIO_PDSR
//#define CC1100_1_IN_PIO_ID	AT91C_ID_PIOA

//PORT 2
//#define CC1100_2_CS_PIN		  10
//#define CC1100_2_CS_BASE	  AT91C_BASE_PIOA
//#define CC1100_2_OUT_PIN    9
//#define CC1100_2_OUT_BASE   AT91C_BASE_PIOA
//#define CC1100_2_OUT_PORT   AT91C_BASE_PIOA->PIO_PDSR
//#define CC1100_2_IN_PIN     11
//#define CC1100_2_IN_BASE	  AT91C_BASE_PIOA
//#define CC1100_2_IN_PORT    AT91C_BASE_PIOA->PIO_PDSR
//#define CC1100_2_IN_PIO_ID	AT91C_ID_PIOA

//PORT 3
//#define CC1100_3_CS_PIN		  24
//#define CC1100_3_CS_BASE	  AT91C_BASE_PIOB
//#define CC1100_3_OUT_PIN    20
//#define CC1100_3_OUT_BASE   AT91C_BASE_PIOB
//#define CC1100_3_OUT_PORT   AT91C_BASE_PIOB->PIO_PDSR
//#define CC1100_3_IN_PIN     25
//#define CC1100_3_IN_BASE	  AT91C_BASE_PIOB
//#define CC1100_3_IN_PORT    AT91C_BASE_PIOB->PIO_PDSR
//#define CC1100_3_IN_PIO_ID	AT91C_ID_PIOB

/*
#define CCCOUNT				4
#define CCTRANSCEIVERS		{{CC1100_0_CS_BASE, CC1100_0_CS_PIN, CC1100_0_IN_BASE, CC1100_0_IN_PIN},\
							 {CC1100_1_CS_BASE, CC1100_1_CS_PIN, CC1100_1_IN_BASE, CC1100_1_IN_PIN},\
							 {CC1100_2_CS_BASE, CC1100_2_CS_PIN, CC1100_2_IN_BASE, CC1100_2_IN_PIN},\
							 {CC1100_3_CS_BASE, CC1100_3_CS_PIN, CC1100_3_IN_BASE, CC1100_3_IN_PIN}}
*/

//default Transceiver
#define CC1100_CS_PIN		    CC1100_0_CS_PIN
#define CC1100_CS_GPIO		  CC1100_0_CS_GPIO
#define CC1100_OUT_PIN      CC1100_0_OUT_PIN
#define CC1100_OUT_GPIO     CC1100_0_OUT_GPIO
#define CC1100_IN_PIN       CC1100_0_IN_PIN
#define CC1100_IN_GPIO     	CC1100_0_IN_GPIO
#define CC1100_IN_PORT  	  CC1100_0_IN_PORT
#define CC1100_OUT_IN       CC1100_0_OUT_PORT
#define CC1100_IN_IN        CC1100_0_IN_PORT

//#define HAS_ETHERNET            1       // undef or define...1
//#define HAS_ETHERNET_KEEPALIVE  1
//#define ETHERNET_KEEPALIVE_TIME 30
//#define HAS_NTP                 1       // undef or define...1

#ifdef MapleCUN
#define HAS_W5100
#endif

//------------------------------------------------------------------------------
//         Headers
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//         Pin Definitions
//------------------------------------------------------------------------------
#define LED_GPIO              GPIOB
#define LED_PIN               1

#define LED2_GPIO             GPIOB
#define LED2_PIN              0

#define USBD_CONNECT_PORT     GPIOB
#define USBD_CONNECT_PIN      9

#ifdef MapleCUN

#define WIZNET_CS_PIN         12
#define WIZNET_CS_GPIO        GPIOB

#define WIZNET_RST_PIN        8
#define WIZNET_RST_GPIO       GPIOA

#endif

#endif //#ifndef BOARD_H

