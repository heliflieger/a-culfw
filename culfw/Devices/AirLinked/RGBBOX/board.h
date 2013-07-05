#ifndef DEVICE_H
#define DEVICE_H

#include "../board.h.zCSM"

#define XLED
#define HAS_PRIVATE_CHANNEL

#define HAS_IRTX                		//IR-Transmission
#define IRTX_CALLBACKS 
#ifndef F_INTERRUPTS
#define F_INTERRUPTS            15625   	// interrupts per second, min: 10000, max: 20000
#endif

#define REPTIME                 20 

#define LED_DDR                 DDRC
#define LED_PORT                PORTC
#define LED_PIN                 4

#define KEY_DDR                 DDRC
#define KEY_PORT                PORTC
#define KEY_PIN                 3
#define KEY_IN                  PINC

#endif

