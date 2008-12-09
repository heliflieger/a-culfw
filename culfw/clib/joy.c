#include "board.h"
#include "joy.h"
#include "delay.h"
#include "clock.h"
#include "display.h"
#include "pcf8833.h"
#include <avr/interrupt.h>

void (*joyfunc)(uint8_t) = 0;
uint8_t joy_last_sec, joy_last_hsec, joy_last_key, joy_repeat;
uint8_t joy_inactive;

void
joy_init(void)
{
  JOY_DDR1 &= ~(_BV(JOY_PIN1) | _BV(JOY_PIN2) | _BV(JOY_PIN3));
  JOY_DDR2 &= ~(_BV(JOY_PIN4) | _BV(JOY_PIN5));

  JOY_OUT_PORT1 |= (_BV(JOY_PIN1) | _BV(JOY_PIN2) | _BV(JOY_PIN3));
  JOY_OUT_PORT2 |= (_BV(JOY_PIN4) | _BV(JOY_PIN5));
}

static int
check_key(void)
{
  uint8_t key = KEY_NONE;
       if(!bit_is_set( JOY_IN_PORT1, JOY_PIN1)) key = KEY_ENTER;
  else if(!bit_is_set( JOY_IN_PORT1, JOY_PIN2)) key = KEY_RIGHT;
  else if(!bit_is_set( JOY_IN_PORT1, JOY_PIN3)) key = KEY_LEFT;
  else if(!bit_is_set( JOY_IN_PORT2, JOY_PIN4)) key = KEY_UP;
  else if(!bit_is_set( JOY_IN_PORT2, JOY_PIN5)) key = KEY_DOWN;

  return key;
}

TASK(JOY_Task)
{
  uint8_t key = check_key();

  if(key == KEY_NONE) {
    joy_last_key = KEY_NONE;
    joy_repeat = 0;
    return;
  }

  if(key == joy_last_key) {

    int16_t tdiff = sec-joy_last_sec;
    if(tdiff < 0)
      tdiff += 60;
    if(tdiff < 2) {
      tdiff = (tdiff ? 125 : 0) + hsec-joy_last_hsec;
      if(tdiff < (joy_repeat ? 12 : 62))   // Repeat: 0.1 sec / First: 0.5 sec
        return;
    }
    joy_repeat = 1;

  } else {

    joy_repeat = 0;
  }

  if(joyfunc)
    joyfunc(key);

  joy_inactive = 0;
  joy_last_sec = sec;
  joy_last_hsec = hsec;
  joy_last_key = key;
}


void
joy_enable_interrupts(void)
{
  JOY_INTREG &= ~(JOY_INT1ISC|JOY_INT2ISC);       // Low level
  EIMSK |= (_BV(JOY_INT1)|_BV(JOY_INT2));
  EIFR  |= (_BV(JOY_INT1)|_BV(JOY_INT2));
}
void
joy_disable_interrupts(void)
{
  EIMSK &= ~(_BV(JOY_INT1)|_BV(JOY_INT2));
}


// Only used to wake up from sleep.
ISR(JOY_INT1VECT) { }
ISR(JOY_INT2VECT) { }

