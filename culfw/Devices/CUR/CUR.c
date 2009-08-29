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

df_chip_t df;

PROGMEM t_fntab fntab[] = {

  { 'B', prepare_boot },
  { 'C', ccreg },
  { 'F', fs20send },
#ifdef HAS_RAWSEND
  { 'G', rawsend },
#endif
  { 'M', more },
  { 'P', lcd_drawpic },
  { 'R', read_eeprom },
  { 'T', fhtsend },
  { 'V', version },
  { 'W', write_eeprom },
  { 'X', set_txreport },

  { 'a', batfunc },
  { 'c', rtcfunc },
  { 'e', eeprom_factory_reset },
#ifdef HAS_FASTRF
  { 'f', fastrf },
#endif
  { 'd', lcdfunc },
  { 'l', ledfunc },
  { 'r', read_file },
  { 's', mysleep },
  { 't', gettime },
  { 'w', write_file },
  { 'x', ccsetpa },

  { 0, 0 },
};

#if defined(__AVR_AT90USB1286__)
#define jump_to_bootloader ((void(*)(void))0xf000)
#elif defined(__AVR_AT90USB646__)
#define jump_to_bootloader ((void(*)(void))0x7800)
#else
#define jump_to_bootloader ((void(*)(void))0x1800)
#endif

void
dpy(char *in)
{
  uint8_t bl = 0;
  fromhex(in+1, &bl, 1);

}



void
start_bootloader(void)
{
  cli();
  
  /* move interrupt vectors to bootloader section and jump to bootloader */
  MCUCR = _BV(IVCE);
  MCUCR = _BV(IVSEL);

  jump_to_bootloader();
}

int
main(void)
{
  spi_init();
  eeprom_init();

  // if we had been restarted by watchdog check the REQ BootLoader byte in the
  // EEPROM ...
  if (bit_is_set(MCUSR,WDRF) && eeprom_read_byte(EE_REQBL)) {
     eeprom_write_byte( EE_REQBL, 0 ); // clear flag
     start_bootloader();
  }


  // Setup the timers. 

  // Timer0 is used by the main clock
  OCR0A  = 249;                            // Timer0: 0.008s = 8MHz/256/250
  TCCR0B = _BV(CS02);       
  TCCR0A = _BV(WGM01);
  TIMSK0 = _BV(OCIE0A);

  // Timer1 is used by my_delay and by the transceiver analyzer
  TCCR1A = 0;
  TCCR1B = _BV(CS11) | _BV(WGM12);         // Timer1: 1us = 8MHz/8

#ifdef CURV3
  // Timer3 is used by the LCD backlight (PWM)
  TCCR3A =  _BV(COM3A1)| _BV(WGM30);       // Fast PWM, 8-bit, clear on match
  TCCR3B =  _BV(WGM32) | _BV(CS31);        // Prescaler 8MHz/8: 3.9KHz 
#endif

  clock_prescale_set(clock_div_1);         // Disable Clock Division

  MCUSR &= ~(1 << WDRF);                   // Enable the watchdog
  wdt_enable(WDTO_2S); 

  led_init();
  LED_ON();
  rb_init(USB_Tx_Buffer, CDC_TX_EPSIZE);
  rb_init(USB_Rx_Buffer, CDC_RX_EPSIZE);
  USB_Init();
  tty_init();

                                // lcd init is done manually from menu_init
  joy_init();
  bat_init();
  df_init(&df);
  fs_init(&fs, df, 0);          // needs df_init
  rtc_init();                   // does i2c_init too
  menu_init();                  // needs fs_init
  fht_init();
  log_init();                   // needs fs_init & rtc_init
  tx_init();
  output_enabled = OUTPUT_USB|OUTPUT_LCD|OUTPUT_LOG;
  //Log("Boot");

  LED_OFF();

  for(;;) {
    USB_USBTask();
    CDC_Task();
    RfAnalyze_Task();
    Minute_Task();
    if(fastrf_on)
      FastRF_Task();
    JOY_Task();
  }
}
