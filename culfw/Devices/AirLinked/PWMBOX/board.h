#ifndef DEVICE_H
#define DEVICE_H

#include "../board.h.zCSM"

#define XADB2001
#define HAS_PRIVATE_CHANNEL
#define XLED

#define REPTIME                 20 

#undef LED_DDR
#undef LED_PORT
#undef LED_PIN

#ifdef ADB2001

#define LED_DDR                 DDRC
#define LED_PORT                PORTC
#define LED_PIN                 0
#define LED_INV

#define KEY_DDR                 DDRD
#define KEY_PORT                PORTD
#define KEY_PIN                 3
#define KEY_IN                  PIND

#define REL_DDR                 DDRC
#define REL_PORT                PORTC
#define REL_PIN                 1

#else

#define LED_DDR                 DDRC
#define LED_PORT                PORTC
#define LED_PIN                 4

#define KEY_DDR                 DDRC
#define KEY_PORT                PORTC
#define KEY_PIN                 3
#define KEY_IN                  PINC

#endif

#endif

