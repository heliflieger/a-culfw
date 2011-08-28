#include "actions.h"
#include "board.h"
#include "util.h"
#include "config.h"
#include "cc1100.h"
#include "fs20.h"
#include <util/delay.h>
#include <avr/eeprom.h>

volatile uint16_t housecode;
volatile uint8_t button;

uint16_t	cfgOffsetHousecode	EEMEM;
uint8_t		cfgOffsetButton		EEMEM;

#define FS20_CODE_ON	0x11
#define FS20_CODE_OFF	0x00

/*
 * 
 */
void fs20_sendsensor(void) {
  
  // Sensor 
  #define SENSOR_PORT		PINA
  #define SENSOR_DDR		DDRA
  #define SENSOR_PIN		PA1
  // Sensor is input, activate internal pullup resistor
  // #define SENSOR_INIT()		SENSOR_DDR&=~_BV(SENSOR_PIN);SENSOR_PORT|=_BV(SENSOR_PIN);
  // Sensor is input, no internal pullup resistor
  #define SENSOR_INIT()		SENSOR_DDR&=~_BV(SENSOR_PIN);
  #define SENSOR_STATE() 	(!bit_is_set(SENSOR_PORT,SENSOR_PIN))
  
  // initialize sensor
  SENSOR_INIT();
  // wait some time to debounce sensor
  _delay_ms(5);
  // get sensor
  uint8_t on= SENSOR_STATE();
  // send command
  fs20_sendCommand(housecode, button, on ? FS20_CODE_ON : FS20_CODE_OFF);
  // fiddling with the LED breaks the transfer in cases
  // when the LED is connected to PA2! 
  _delay_ms(250); // wait some time for transfer to finish
  // a powerdown might break the transfer!

}

void fs20_getconfig(void) {
  // init variables from eeprom
  housecode = get_config_word(&cfgOffsetHousecode);
  button = get_config_byte(&cfgOffsetButton);
}

void fs20_setconfig(uint16_t newhousecode, uint8_t newbutton) {
  housecode= newhousecode;
  button= newbutton;
  set_config_word (&cfgOffsetHousecode, housecode);
  set_config_byte (&cfgOffsetButton, button);
}
 
void fs20_reception(void) {
  
  ccInitChip();
  
  fs20_resetbuffer();
  fs20_rxon();

  fstelegram_t t;
  t.type = 'U'; // undefined
  uint16_t rxtimeout = 300; // 300*100ms= 30s

  // wait for next telegram
  // exit if button pressed or timeout occured
  while(!fs20_readFS20Telegram(&t) && (rxtimeout--)>0) {
    _delay_ms(100);
    if(LED_PIN!=PA3) LED_TOGGLE(); // do not touch pin for CC1100 communication
  }
  fs20_idle();
  

  // save the result
  if(t.type=='F')   {
	  fs20_setconfig(t.housecode, t.button);
	  led_blink(3); // confirm save of config
	  fs20_sendsensor(); // force transmit
  }
}
 

/*
 * 
 */
void action_getconfig(void) {
  fs20_getconfig();
}

void action_keypressshort(void) {
  fs20_sendsensor();
}

void action_keypresslong(void) {
  fs20_reception();
  LED_OFF();
}

void action_timeout(void) {
  fs20_sendsensor();
}

void action_pinchanged(void) {
  fs20_sendsensor();	// send the new state
 // _delay_ms(2000); 	// wait two seconds
 // fs20_sendsensor();	// and send it again
 
}

