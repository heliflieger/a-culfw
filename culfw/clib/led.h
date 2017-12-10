#ifndef _LED_H
#define _LED_H   1

#define HI8(x)  ((uint8_t)((x) >> 8))
#define LO8(x)  ((uint8_t)(x))

#ifndef SET_BIT
#define SET_BIT(PORT, BITNUM) ((PORT) |= (1<<(BITNUM)))
#endif
#ifndef CLEAR_BIT
#define CLEAR_BIT(PORT, BITNUM) ((PORT) &= ~(1<<(BITNUM)))
#endif
#define TOGGLE_BIT(PORT, BITNUM) ((PORT) ^= (1<<(BITNUM)))

#include "board.h"

#ifdef USE_HAL

#include "hal.h"

#define led_init()      HAL_LED_Init()

#define LED_ON()        HAL_LED_Set(0,LED_on)
#define LED_OFF()       HAL_LED_Set(0,LED_off)
#define LED_TOGGLE()    HAL_LED_Toggle(0)

#define LED2_ON()       HAL_LED_Set(1,LED_on)
#define LED2_OFF()      HAL_LED_Set(1,LED_off)
#define LED2_TOGGLE()   HAL_LED_Toggle(1)

#define LED3_ON()       HAL_LED_Set(2,LED_on)
#define LED3_OFF()      HAL_LED_Set(2,LED_off)
#define LED3_TOGGLE()   HAL_LED_Toggle(2)


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
