#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/eeprom.h>
#include <avr/sleep.h>
#include "mysleep.h"
#include "display.h"
#include "pcf8833.h"
#include "joy.h"
#include "led.h"
#include "battery.h"
#include "transceiver.h"
#include "fncollection.h"


uint8_t sleep_time;

void
mysleep(char *in)
{
  fromhex(in, &sleep_time, 1);
  if(sleep_time == 0xFF)
    lcdfunc("dff00");
  eeprom_write_byte(EE_SLEEPTIME, sleep_time);
}

void
dosleep(void)
{
  LED_OFF();
  wdt_disable(); 
  joy_enable_interrupts();
  lcdfunc("dff00");
  set_txoff();


  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  cli();
  sleep_enable();
  sei();
  sleep_cpu();


  // Wake-up sequence
  sleep_disable();
  joy_disable_interrupts();
  sei();
  wdt_enable(WDTO_2S); 
  lcdfunc("dff01ff");
  bat_drawstate();
  set_txrestore();
}
