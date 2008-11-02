#include "joy.h"
#include "delay.h"
#include "clock.h"

void (*joyfunc)(uint8_t) = 0;
uint8_t joy_last_min, joy_last_sec, joy_last_hsec, joy_last_key, joy_repeat;

void
joy_init(void)
{
  JOY_DDR1 = 0;
  JOY_DDR2 = 0;

  JOY_PORT1 |= (_BV(JOY_PIN1) | _BV(JOY_PIN2) | _BV(JOY_PIN3));
  JOY_PORT2 |= (_BV(JOY_PIN4) | _BV(JOY_PIN5));
}

TASK(JOY_Task)
{
  uint8_t key = KEY_NONE;

       if(!bit_is_set( JOY_PINSET_1, JOY_PIN1)) key = KEY_ENTER;
  else if(!bit_is_set( JOY_PINSET_1, JOY_PIN2)) key = KEY_RIGHT;
  else if(!bit_is_set( JOY_PINSET_1, JOY_PIN3)) key = KEY_LEFT;
  else if(!bit_is_set( JOY_PINSET_2, JOY_PIN4)) key = KEY_UP;
  else if(!bit_is_set( JOY_PINSET_2, JOY_PIN5)) key = KEY_DOWN;

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
      if(tdiff < (joy_repeat ? 12 : 62))                // Repeat: 0.1 sec / First: 0.5 sec
        return;
    }
    joy_repeat = 1;

  } else {

    joy_repeat = 0;
  }

  if(joyfunc)
    joyfunc(key);

  joy_last_min = minute;                // Used for display off
  joy_last_sec = sec;
  joy_last_hsec = hsec;
  joy_last_key = key;

}
