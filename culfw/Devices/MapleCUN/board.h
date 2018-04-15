#ifndef BOARD_H 
#define BOARD_H

#define _BV(a) (1<<(a))
#define bit_is_set(sfr, bit) ((sfr) & _BV(bit))

#define LONG_PULSE

#define TTY_BUFSIZE          512      // RAM: TTY_BUFSIZE*4

#if defined MapleCUNx4 || defined MapleCUNx4_BL
#define BOARD_NAME          "MapleCUNx4"
#define BOARD_ID_STR        "MapleCUNx4"
#define HAS_MULTI_CC        4
#define NUM_SLOWRF          3
#define HAS_WIZNET

#elif defined MapleCUNx4_W5100 || defined MapleCUNx4_W5100_BL
#define BOARD_NAME          "MapleCUNx4"
#define BOARD_ID_STR        "MapleCUNx4"
#define HAS_MULTI_CC        4
#define NUM_SLOWRF          3
#define HAS_WIZNET
#define _WIZCHIP_           5100
#define USE_HW_AUTODETECT

#elif defined MapleCUNx4_W5500 || defined MapleCUNx4_W5500_BL
#define BOARD_NAME          "MapleCUNx4"
#define BOARD_ID_STR        "MapleCUNx4"
#define HAS_MULTI_CC        4
#define NUM_SLOWRF          3
#define HAS_WIZNET
#define _WIZCHIP_           5500
#define USE_HW_AUTODETECT

#elif defined MapleCULx4 || defined MapleCULx4_BL
#define BOARD_NAME          "MapleCULx4"
#define BOARD_ID_STR        "MapleCULx4"
#define HAS_MULTI_CC        4
#define NUM_SLOWRF          3
#define USE_HW_AUTODETECT

#elif defined MapleCUN || defined MapleCUN_BL
#define BOARD_NAME          "MapleCUN"
#define BOARD_ID_STR        "MapleCUN"
#define HAS_WIZNET

#elif defined MapleCUL || defined MapleCUL_BL
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
#define UART_BAUD_RATE          115200
#define HAS_UART                1
#define USE_RF_MODE
#define USE_HAL
#define HAS_ONEWIRE             10        // OneWire Support
#define MAX_CREDIT 3600       // max 36 seconds burst / 100% of the hourly budget

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
#define LACROSSE_HMS_EMU
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

//PORT 2
#define CC1100_2_CS_PIN     7
#define CC1100_2_CS_BASE    GPIOB
#define CC1100_2_OUT_PIN    14
#define CC1100_2_OUT_BASE   GPIOC
#define CC1100_2_IN_PIN     6
#define CC1100_2_IN_BASE    GPIOB

//PORT 3
#define CC1100_3_CS_PIN     0
#define CC1100_3_CS_BASE    GPIOB
#define CC1100_3_OUT_PIN    0
#define CC1100_3_OUT_BASE   NULL
#define CC1100_3_IN_PIN     13
#define CC1100_3_IN_BASE    GPIOC

#define CCCOUNT             4
#define CCTRANSCEIVERS    {\
                          { {CC1100_0_OUT_BASE, CC1100_0_CS_BASE, CC1100_0_IN_BASE},\
                            {CC1100_0_OUT_PIN,  CC1100_0_CS_PIN,  CC1100_0_IN_PIN}  },\
                          { {CC1100_1_OUT_BASE, CC1100_1_CS_BASE, CC1100_1_IN_BASE},\
                            {CC1100_1_OUT_PIN,  CC1100_1_CS_PIN,  CC1100_1_IN_PIN}  },\
                          { {CC1100_2_OUT_BASE, CC1100_2_CS_BASE, CC1100_2_IN_BASE},\
                            {CC1100_2_OUT_PIN,  CC1100_2_CS_PIN,  CC1100_2_IN_PIN}  },\
                          { {CC1100_3_OUT_BASE, CC1100_3_CS_BASE, CC1100_3_IN_BASE},\
                            {CC1100_3_OUT_PIN,  CC1100_3_CS_PIN,  CC1100_3_IN_PIN}  },\
                          }

//TWI
#define TWI_SCL_PIN       CC1100_2_IN_PIN
#define TWI_SCL_BASE      CC1100_2_IN_BASE
#define TWI_SDA_PIN       CC1100_2_CS_PIN
#define TWI_SDA_BASE      CC1100_2_CS_BASE

#ifdef HAS_WIZNET
#ifndef _WIZCHIP_
#define _WIZCHIP_      5100
#endif
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

//#define LED2_GPIO             GPIOB
//#define LED2_PIN              0

#define USBD_CONNECT_PORT     GPIOB
#define USBD_CONNECT_PIN      9

#ifdef HAS_WIZNET

#define WIZNET_CS_PIN         12
#define WIZNET_CS_GPIO        GPIOB

#define WIZNET_RST_PIN        8
#define WIZNET_RST_GPIO       GPIOA

#endif

#endif //#ifndef BOARD_H

