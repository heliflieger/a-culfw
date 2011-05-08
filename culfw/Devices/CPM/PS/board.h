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
 * LED between PB0 and VDD
 */
#define LED_PORT 	PORTB
#define LED_DDR		DDRB
#define LED_PIN 	PB0
#define LED_INIT()   	LED_DDR  |= _BV(LED_PIN)
#define LED_OFF()    	LED_PORT |= _BV(LED_PIN)
#define LED_ON()     	LED_PORT &= ~_BV(LED_PIN)
#define LED_TOGGLE() 	LED_PORT ^= _BV(LED_PIN)


/* 
 * LED between PA2 and GND 
 * 
#define LED_PORT 	PORTA
#define LED_DDR		DDRA
#define LED_PIN 	PA2
#define LED_INIT() 	LED_DDR|=_BV(LED_PIN)
#define LED_ON() 	LED_PORT|=_BV(LED_PIN)
#define LED_OFF() 	LED_PORT&=~_BV(LED_PIN)
#define LED_TOGGLE() 	LED_PORT^=_BV(LED_PIN)

*/

/*
 * Switch
 */
#define SW_PORT		PINA
#define SW_DDR		DDRA
#define SW_PIN		PA1
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
#define MASK0		_BV (PCINT1) 

/*
 * Globals
 */

// If USE_TIMER is defined the power down/watchdog mechanism is disabled.
// This leads to a sufficient accuracy if commands need to be send out
// in fixed intervals. Do not forget to calibrate the timing mechanism 
// for your device by adjusting TIMERCORR below.
#define USE_TIMER 1

// timer correction
// the 8s watchdog wakes up every 9.3 seconds approximately
// #define TIMERCORR 700/600
// the internal oscillator gains about 610 seconds instead of 600
#define TIMERCORR 101587/100000

// time in seconds after which action_timeout is triggered, 1800s= 30min
#define CLOCKTIMEOUT	(600)
// time in milliseconds after which a long key press is detected
#define KEYTIMEOUT 	(5000)

#endif /* BOARD_H_ */
