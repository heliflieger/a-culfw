/*


*/

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <avr/power.h> // prescaler
#include "board.h"
#include "config.h"
#include "util.h"
#include "fs20.h"
#include "cc1100.h"
#include "actions.h"

volatile uint8_t pinchanged= 0;

int
main (void)
{
  clock_prescale_set(clock_div_1); // 8 MHz
  reset_clock();
  hardware_init();
  action_getconfig();
  led_blink(5);
  
  while(1) {
    // every time the device wakes up it gets HERE
    switch(get_keypress(KEYTIMEOUT)) {
      case 0: // no key press
	if(get_clock()>=(CLOCKTIMEOUT)) {
	  action_timeout();
	  reset_clock();
	};
	if(pinchanged) {
	  action_pinchanged();
	  pinchanged= 0;
	}
	break;
      case 1: // short key press
	action_keypressshort();
	break;
      case 2: // long key press
	action_keypresslong();
	break;
    }
#ifndef USE_TIMER
    power_down();
#endif
  }
}

ISR (PCINT0_vect)
{
  // This interrupt is only used to wake up the microcontroller on level changes of the input pins
  pinchanged= 1;
}
  

ISR (PCINT1_vect)
{
  // This interrupt is only used to wake up the microcontroller on level changes of the input pins
  // No action needed here.
  pinchanged= 1;
}

ISR (WATCHDOG_vect)
{
  // This interrupt service routine is called every WDTIMEOUT seconds (see utils.c)
  tick_clock();
}

// timer interrupt, see http://www.nongnu.org/avr-libc/user-manual/group__avr__interrupts.html
ISR(TIM0_COMPA_vect, ISR_BLOCK)
{
  // 125Hz
  tick_tick();
}

