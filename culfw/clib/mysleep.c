#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/sleep.h>
#include "mysleep.h"
#include "display.h"
#include "pcf8833.h"
#include "joy.h"
#include "led.h"
#include "battery.h"
#include "transceiver.h"


uint8_t sleep_time;

void
mysleep(char *in)
{
  fromhex(in, &sleep_time, 1);
}

void
dosleep(void)
{
  LED_OFF();
  wdt_disable(); 
  joy_enable_interrupts();
  lcdfunc("dff00");
  set_txreport("X00");

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
}
