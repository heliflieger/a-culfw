#ifndef _BOARD_H
#define _BOARD_H

#define BOARD_ID_STR            "CUR"
#define BOARD_ID_USTR           L"CUR"
#define BUSWARE_CUR

#define HAS_USB
#define HAS_LCD
#define HAS_FS
#define HAS_SLEEP
#define HAS_BATTERY
#define HAS_RTC



#define TTY_BUFSIZE             64
#define USB_BUFSIZE             64      // Must be a supported USB endpoint size
#define HAS_FHT_8v                      // PROGMEM:  586b, RAM: 23b
#define HAS_FHT_80b                     // PROGMEM: 1374b, RAM: 90b
#define HAS_FHT_TF
#define FHTBUF_SIZE             200     //                 RAM:200b
#define HAS_RF_ROUTER           1       // PROGMEM: 1106b, RAM: 43b
#define RCV_BUCKETS             4       //                 RAM: 25b / bucket

#define FULL_CC1100_PA                  // PROGMEM:  100b
#define HAS_FASTRF                      // PROGMEM:  362b  RAM:  1b
#define HAS_RAWSEND                     // PROGMEM:   90b  RAM:  6b
#define HAS_ESA
#define HAS_TX3
#define HAS_HOERMANN
#define HAS_CC1101_RX_PLL_LOCK_CHECK_TASK_WAIT
#define HAS_CC1101_PLL_LOCK_CHECK_MSG
#define HAS_CC1101_PLL_LOCK_CHECK_MSG_SW

#ifdef CURV3
#  include "board_v3.h"
#else
#  include "board_v2.h"
#endif

#endif
