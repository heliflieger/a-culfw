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
//#define USB_FIX_SERIAL          "012345"
#define CDC_COUNT               3
#define CDC_BAUD_RATE           115200
#define USB_IsConnected		      (CDC_isConnected(0))
#define HAS_XRAM

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
#define HAS_HOERMANN_SEND
#define HAS_SOMFY_RTS
#define HAS_MAICO
#define HAS_RFNATIVE
#define HAS_ZWAVE
#define HAS_MBUS

#define _433MHZ

#  if defined(_433MHZ)
#    define HAS_TCM97001
#    define HAS_IT
#    define HAS_HOMEEASY
#    define HAS_BELFOX
#    define HAS_MANCHESTER
#    define HAS_REVOLT
#  endif



//additional CC1101 Transceiver
//#define CC1100_ASKSIN		1
//#define CC1100_MORITZ		2
//#define CC1100_MAICO		3

//PORT 0
#define CC1100_0_CS_PIN		  4
#define CC1100_0_CS_BASE	  GPIOA
#define CC1100_0_OUT_PIN    1
#define CC1100_0_OUT_BASE   GPIOA
#define CC1100_0_IN_PIN     0
#define CC1100_0_IN_BASE    GPIOA

//PORT 1
#define CC1100_1_CS_PIN		  15
#define CC1100_1_CS_BASE	  GPIOC
#define CC1100_1_OUT_PIN    5
#define CC1100_1_OUT_BASE   GPIOB
#define CC1100_1_IN_PIN     4
#define CC1100_1_IN_BASE	  GPIOB


#define CCCOUNT       2
#define CCTRANSCEIVERS    {\
                          { {CC1100_0_OUT_BASE, CC1100_0_CS_BASE, CC1100_0_IN_BASE},\
                            {CC1100_0_OUT_PIN,  CC1100_0_CS_PIN,  CC1100_0_IN_PIN}  },\
                          { {CC1100_1_OUT_BASE, CC1100_1_CS_BASE, CC1100_1_IN_BASE},\
                            {CC1100_1_OUT_PIN,  CC1100_1_CS_PIN,  CC1100_1_IN_PIN}  },\
                          }

#ifdef MapleCUN
#define HAS_W5100
#endif

#ifndef CDC_COUNT
#define CDC_COUNT 1
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

