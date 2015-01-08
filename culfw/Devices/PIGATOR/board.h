#ifndef _PIGBOARD_H
#define _PIGBOARD_H

#include "../COC/board.h"

#undef  HAS_ONEWIRE
#undef  HAS_RTC

#undef LED_DDR
#undef LED_PORT
#undef LED_PIN

#define LED_DDR                 DDRD
#define LED_PORT                PORTD
#define LED_PIN                 4

#define MULTI_FREQ_DEVICE       // available in multiple versions: 433MHz,868MHz,915MHz

#define MARK433_PORT            PORTC
#define MARK433_PIN             PINC   
#define MARK433_BIT             4

#define MARK915_PORT            PORTC
#define MARK915_PIN             PINC    
#define MARK915_BIT             5

#define BOARD_ID_STR915         "CSM915"


#endif

