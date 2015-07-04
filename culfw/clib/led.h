#ifndef _LED_H
#define _LED_H   1

#define HI8(x)  ((uint8_t)((x) >> 8))
#define LO8(x)  ((uint8_t)(x))

#define SET_BIT(PORT, BITNUM) ((PORT) |= (1<<(BITNUM)))
#define CLEAR_BIT(PORT, BITNUM) ((PORT) &= ~(1<<(BITNUM)))
#define TOGGLE_BIT(PORT, BITNUM) ((PORT) ^= (1<<(BITNUM)))

#include "board.h"

#ifdef ARM

#include <pio/pio.h>

static const Pin pinsLeds[] = {PINS_LEDS};

#define LED_ON()    	PIO_Clear(&pinsLeds[0])
#define LED_OFF()     	PIO_Set(&pinsLeds[0])
#define LED_TOGGLE()	{if (PIO_GetOutputDataStatus(&pinsLeds[0])) {PIO_Clear(&pinsLeds[0]);} else {PIO_Set(&pinsLeds[0]);}}

#ifdef CUBE
#define led_init() 		PIO_Configure(&pinsLeds[0], 1);PIO_Configure(&pinsLeds[1], 1);PIO_Configure(&pinsLeds[2], 1)

#define LED2_ON()    	PIO_Clear(&pinsLeds[1])
#define LED2_OFF()     	PIO_Set(&pinsLeds[1])

#define LED3_ON()    	PIO_Clear(&pinsLeds[2])
#define LED3_OFF()     	PIO_Set(&pinsLeds[2])

#define LED2_TOGGLE()	{if (PIO_GetOutputDataStatus(&pinsLeds[1])) {PIO_Clear(&pinsLeds[1]);} else {PIO_Set(&pinsLeds[1]);}}
#define LED3_TOGGLE()	{if (PIO_GetOutputDataStatus(&pinsLeds[2])) {PIO_Clear(&pinsLeds[2]);} else {PIO_Set(&pinsLeds[2]);}}
#else
#define led_init() 		PIO_Configure(&pinsLeds[0], 1)

#define LED2_ON()
#define LED2_OFF()

#define LED3_ON()
#define LED3_OFF()

#define LED2_TOGGLE()
#define LED3_TOGGLE()
#endif

#else

#ifdef XLED
#include "xled.h"
#define led_init()   LED_DDR  |= _BV(LED_PIN); xled_pos=0; xled_pattern=0xff00
#else
#define led_init()   LED_DDR  |= _BV(LED_PIN)
#endif

#define LED_TOGGLE() LED_PORT ^= _BV(LED_PIN)

#ifdef LED_INV
#define LED_OFF()    LED_PORT |= _BV(LED_PIN)
#define LED_ON( )    LED_PORT &= ~_BV(LED_PIN)
#else
#define LED_ON()     LED_PORT |= _BV(LED_PIN)
#define LED_OFF( )   LED_PORT &= ~_BV(LED_PIN)
#endif

#endif
#endif
