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

#undef  HAS_FHT_8v              // PROGMEM:  434b, MEM: 19b
#define HAS_FHT_80b             // PROGMEM: 1158b, MEM:  5b+FHTBUF_SIZE
#define FHTBUF_SIZE             128
#define FULL_CC1100_PA          //  100 byte PROGMEM
#define FHTBUF_MODEL1           // see fht.c for details
#define RCV_BUCKETS             4  // *25b. Syslog is slow, needs more than CUL

#ifdef CURV3
#  include "board_v3.h"
#else
#  include "board_v2.h"
#endif

#endif
