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

#define TTY_BUFSIZE             64      // Must be a supported USB endpoint size
#define HAS_FHT_8v                      // PROGMEM:  584b, RAM: 23b
#define HAS_FHT_80b                     // PROGMEM: 1360b, RAM: 16b
#define FHTBUF_SIZE            200      //                 RAM: 74b

#define FULL_CC1100_PA                  // PROGMEM:  100b
#define RCV_BUCKETS             4       //                 RAM: 25b / bucket

#define HAS_RAWSEND                     // PROGMEM:   90b  RAM:  6b
#define HAS_FASTRF                      // PROGMEM:  274b  RAM:  4b
#define HAS_LONGMSG                     // CUR support     RAM: 20b


#ifdef CURV3
#  include "board_v3.h"
#else
#  include "board_v2.h"
#endif

#endif
