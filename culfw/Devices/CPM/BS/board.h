#ifndef BOARD_H_
#define BOARD_H_

#include <avr/io.h>
#include "cpmboard.h"

/*
 * Here you define how your CPM is connected to
 * the outside world. This is specific to your
 * circuit design.
 */


/*
 * LED between PA3 and VDD
 */
#define LED_PORT 	PORTA
#define LED_DDR		DDRA
#define LED_PIN 	PA3
#define LED_INIT()   	LED_DDR  |= _BV(LED_PIN)
#define LED_OFF()    	LED_PORT |= _BV(LED_PIN)
#define LED_ON()     	LED_PORT &= ~_BV(LED_PIN)
#define LED_TOGGLE() 	LED_PORT ^= _BV(LED_PIN)

/*
 * Switch
 */
#define SW_PORT		PINA
#define SW_DDR		DDRA
#define SW_PIN		PA0
// SW is input, activate internal pullup resistor
#define SW_INIT()	SW_DDR&=~_BV(SW_PIN);SW_PORT|=_BV(SW_PIN);
#define SW_STATE() 	(!bit_is_set(SW_PORT,SW_PIN))

/*
 * Watch Pins
 */
// watch PA0
//#define MASK0		(_BV (PCINT0) | _BV(PCINT7))
// if there are two of them: put them in brackets
// this is the pin where the switch is connected to
#define MASK0		_BV (PCINT0) 

/*
 * Globals
 */

// Watchdog timer correction
// the 8s watchdog wakes up every 9.3 seconds approximately
#define WDCORR 700/600

// time in seconds after which action_timeout is triggered, 1800s= 30min
#define CLOCKTIMEOUT	(600)
// time in milliseconds after which a long key press is detected
#define KEYTIMEOUT 	(5000)

#endif /* BOARD_H_ */
