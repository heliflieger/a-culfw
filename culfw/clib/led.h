#ifndef _LED_H
#define _LED_H   1

#define HI8(x)  ((uint8_t)((x) >> 8))
#define LO8(x)  ((uint8_t)(x))

#define SET_BIT(PORT, BITNUM) ((PORT) |= (1<<(BITNUM)))
#define CLEAR_BIT(PORT, BITNUM) ((PORT) &= ~(1<<(BITNUM)))
#define TOGGLE_BIT(PORT, BITNUM) ((PORT) ^= (1<<(BITNUM)))

#include "board.h"

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
