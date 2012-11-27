/* Copyright Rudolf Koenig, 2008.
   Released under the GPL Licence, Version 2
   Inpired by the MyUSB USBtoSerial demo, Copyright (C) Dean Camera, 2008.
*/

#include <avr/boot.h>
#include <avr/power.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>

#include <string.h>

#include "cc1100.h"
#include "cdc.h"
#include "clock.h"
#include "delay.h"
#include "display.h"
#include "fncollection.h"
#include "led.h"
#include "ringbuffer.h"
#include "rf_send.h"
#include "rf_receive.h"
#include "ttydata.h"
#include "pcf8833.h"
#include "spi.h"
#include "battery.h"
#include "fswrapper.h"
#include "ttydata.h"
#include "joy.h"
#include "menu.h"
#include "mysleep.h"
#include "spi.h"
#include "ds1339.h"
#include "fht.h"
#include "log.h"
#include "more.h"
#include "fastrf.h"
#include "rf_router.h"

df_chip_t df;

const PROGMEM t_fntab fntab[] = {

  { 'B', prepare_boot },
  { 'C', ccreg },
  { 'F', fs20send },
  { 'G', rawsend },
  { 'M', more },
  { 'P', lcd_drawpic },
  { 'R', read_eeprom },
  { 'T', fhtsend },
  { 'V', version },
  { 'W', write_eeprom },
  { 'X', set_txreport },

  { 'a', batfunc },
  { 'c', rtc_func },
  { 'e', eeprom_factory_reset },
  { 'f', fastrf_func },
  { 'd', lcdfunc },
  { 'l', ledfunc },
  { 'r', read_file },
  { 's', mysleep },
  { 't', gettime },
  { 'u', rf_router_func },
  { 'w', write_file },
  { 'x', ccsetpa },

  { 0, 0 },
};

void
start_bootloader(void)
{
  cli();
  
  /* move interrupt vectors to bootloader section and jump to bootloader */
  MCUCR = _BV(IVCE);
  MCUCR = _BV(IVSEL);

#if defined(__AVR_AT90USB1286__)
#define jump_to_bootloader ((void(*)(void))0xf000)
#elif defined(__AVR_AT90USB646__)
#define jump_to_bootloader ((void(*)(void))0x7800)
#else
#define jump_to_bootloader ((void(*)(void))0x1800)
#endif

  jump_to_bootloader();
}

int
main(void)
{
  wdt_enable(WDTO_2S); 
  clock_prescale_set(clock_div_1);         // Disable Clock Division

  // if we had been restarted by watchdog check the REQ BootLoader byte in the
  // EEPROM ...
  if (bit_is_set(MCUSR,WDRF) && erb(EE_REQBL)) {
     ewb( EE_REQBL, 0 ); // clear flag
     start_bootloader();
  }

  // Setup the timers. Are needed for watchdog-reset
  OCR0A  = 249;                            // Timer0: 0.008s = 8MHz/256/250
  TCCR0B = _BV(CS02);       
  TCCR0A = _BV(WGM01);
  TIMSK0 = _BV(OCIE0A);

  TCCR1A = 0;
  TCCR1B = _BV(CS11) | _BV(WGM12);         // Timer1: 1us = 8MHz/8

#ifdef CURV3
  // Timer3 is used by the LCD backlight (PWM)
  TCCR3A =  _BV(COM3A1)| _BV(WGM30);       // Fast PWM, 8-bit, clear on match
  TCCR3B =  _BV(WGM32) | _BV(CS31);        // Prescaler 8MHz/8: 3.9KHz 
#endif

  MCUSR &= ~(1 << WDRF);                   // Enable the watchdog

  led_init();
  spi_init();
  eeprom_init();
  USB_Init();
  fht_init();
  tx_init();
  joy_init();                   // lcd init is done manually from menu_init
  bat_init();
  df_init(&df);
  fs_init(&fs, df, 0);          // needs df_init
  rtc_init();                   // does i2c_init too
  log_init();                   // needs fs_init & rtc_init
  menu_init();                  // needs fs_init
  input_handle_func = analyze_ttydata;
  log_enabled = erb(EE_LOGENABLED);
  display_channel = DISPLAY_USB|DISPLAY_LCD|DISPLAY_RFROUTER;
  rf_router_init();

  LED_OFF();

  for(;;) {
    USB_USBTask();
    CDC_Task();
    RfAnalyze_Task();
    Minute_Task();
    FastRF_Task();
    rf_router_task();
    JOY_Task();
  }
}
