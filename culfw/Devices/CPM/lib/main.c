/*


*/

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
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
  SetSystemClockPrescaler (0);

  hardware_init();
  reset_clock();
  action_getconfig();
  led_blink(5);
  
  /*
  // Sensor 
  #define SENSOR_PORT		PINA
  #define SENSOR_DDR		DDRA
  #define SENSOR_PIN		PA1
  // SW is input, activate internal pullup resistor
  #define SENSOR_INIT()		SENSOR_DDR&=~_BV(SENSOR_PIN);SENSOR_PORT|=_BV(SENSOR_PIN);
  #define SENSOR_STATE() 	(!bit_is_set(SENSOR_PORT,SENSOR_PIN))  
  SENSOR_INIT();
  while(1) {
    if(SW_STATE()) {
      LED_ON(); } else {
	if(SENSOR_STATE()) {
	  LED_ON(); } else {
	  LED_OFF();
	}
      }
  }  
  */	  
  
  
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
    power_down();
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
  // This interrupt service routine is called every TIMEOUT seconds (see utils.c)
  tick_clock();
}
