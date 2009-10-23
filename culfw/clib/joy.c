#include "board.h"
#include "joy.h"
#include "delay.h"
#include "clock.h"
#include "display.h"
#include "pcf8833.h"
#include <avr/interrupt.h>

void (*joyfunc)(uint8_t) = 0;
uint16_t joy_ltick, joy_last_key, joy_repeat;
uint8_t joy_inactive;

void
joy_init(void)
{

#ifdef CURV3

  JOY_DDR1 &= ~(_BV(JOY_PIN2) | _BV(JOY_PIN3));
  JOY_DDR2 &= ~(_BV(JOY_PIN1) | _BV(JOY_PIN4) | _BV(JOY_PIN5));

  JOY_OUT_PORT1 |= (_BV(JOY_PIN2) | _BV(JOY_PIN3));
  JOY_OUT_PORT2 |= (_BV(JOY_PIN1) | _BV(JOY_PIN4) | _BV(JOY_PIN5));

#else

  JOY_DDR1 &= ~(_BV(JOY_PIN1) | _BV(JOY_PIN2) | _BV(JOY_PIN3));
  JOY_DDR2 &= ~(_BV(JOY_PIN4) | _BV(JOY_PIN5));

  JOY_OUT_PORT1 |= (_BV(JOY_PIN1) | _BV(JOY_PIN2) | _BV(JOY_PIN3));
  JOY_OUT_PORT2 |= (_BV(JOY_PIN4) | _BV(JOY_PIN5));

#endif
}

static int
check_key(void)
{
  uint8_t key = KEY_NONE;
#ifdef CURV3
  if(!bit_is_set( JOY_IN_PORT2, JOY_PIN1)) key = KEY_ENTER;
#else
  if(!bit_is_set( JOY_IN_PORT1, JOY_PIN1)) key = KEY_ENTER;
#endif
  else if(!bit_is_set( JOY_IN_PORT1, JOY_PIN2)) key = KEY_RIGHT;
  else if(!bit_is_set( JOY_IN_PORT1, JOY_PIN3)) key = KEY_LEFT;
  else if(!bit_is_set( JOY_IN_PORT2, JOY_PIN4)) key = KEY_UP;
  else if(!bit_is_set( JOY_IN_PORT2, JOY_PIN5)) key = KEY_DOWN;

  return key;
}

void
JOY_Task(void)
{
  uint8_t key = check_key();

  if(key == KEY_NONE) {
    joy_last_key = KEY_NONE;
    joy_repeat = 0;
    return;
  }

  if(key == joy_last_key) {

    uint16_t tdiff = (uint16_t)ticks-joy_ltick;
    if(tdiff < (joy_repeat ? 12 : 62))   // Repeat: 0.1 sec / First: 0.5 sec
      return;
    joy_repeat = 1;

  } else {

    joy_repeat = 0;
  }

  if(joyfunc)
    joyfunc(key);

  joy_inactive = 0;
  joy_ltick = ticks;
  joy_last_key = key;
}



void
joy_enable_interrupts(void)
{
  JOY_INTREG &= ~(JOY_INT1ISC|JOY_INT2ISC);       // Low level
  EIMSK |= (_BV(JOY_INT1)|_BV(JOY_INT2));
  EIFR  |= (_BV(JOY_INT1)|_BV(JOY_INT2));

#ifdef JOY_PCINTMSK
  PCICR |= PCIE0;
  PCMSK0 |= JOY_PCINTMSK;
#endif
}

void
joy_disable_interrupts(void)
{
  EIMSK &= ~(_BV(JOY_INT1)|_BV(JOY_INT2));
#ifdef JOY_PCINTMSK
  PCMSK0 &= ~JOY_PCINTMSK;
#endif
}


// Only used to wake up from sleep.
ISR(JOY_INT1VECT) { }
ISR(JOY_INT2VECT) { }
#ifdef JOY_PCINTMSK
ISR(PCINT0_vect) { }
#endif
