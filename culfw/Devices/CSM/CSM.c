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

#include "spi.h"
#include "cc1100.h"
#include "clock.h"
#include "delay.h"
#include "display.h"
#include "serial.h"
#include "fncollection.h"
#include "led.h"
#include "ringbuffer.h"
#include "rf_receive.h"
#include "rf_send.h"
#include "ttydata.h"
#include "fht.h"
#include "fastrf.h"
#include "rf_router.h"

PROGMEM t_fntab fntab[] = {

  { 'B', prepare_boot },
  { 'C', ccreg },
  { 'F', fs20send },
#ifdef HAS_RAWSEND
  { 'G', rawsend },
#endif
  { 'R', read_eeprom },
  { 'T', fhtsend },
  { 'V', version },
  { 'W', write_eeprom },
  { 'X', set_txreport },

  { 'e', eeprom_factory_reset },
#ifdef HAS_FASTRF
  { 'f', fastrf_func },
#endif
  { 'l', ledfunc },
  { 't', gettime },
#ifdef HAS_RF_ROUTER
  { 'u', rf_router_func },
#endif
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

#define jump_to_bootloader ((void(*)(void))0x1800)
  jump_to_bootloader();
}

int
main(void)
{
  wdt_disable(); 
  led_init();
  LED_ON();
  
  spi_init();


//  eeprom_factory_reset("xx");
  eeprom_init();

//  while(1);
  
  led_mode = 2;
  
  // if we had been restarted by watchdog check the REQ BootLoader byte in the
  // EEPROM ...
//  if(bit_is_set(MCUSR,WDRF) && eeprom_read_byte(EE_REQBL)) {
//    eeprom_write_byte( EE_REQBL, 0 ); // clear flag
//    start_bootloader();
//  }

  // Setup the timers. Are needed for watchdog-reset
  OCR0A  = 249;                            // Timer0: 0.008s = 8MHz/256/250
  TCCR0B = _BV(CS02);       
  TCCR0A = _BV(WGM01);
  TIMSK0 = _BV(OCIE0A);

  TCCR1A = 0;
  TCCR1B = _BV(CS11) | _BV(WGM12);         // Timer1: 1us = 8MHz/8

  clock_prescale_set(clock_div_1);

//  MCUSR &= ~(1 << WDRF);                   // Enable the watchdog
//  wdt_enable(WDTO_2S); 

  uart_init( UART_BAUD_SELECT_DOUBLE_SPEED(UART_BAUD_RATE,F_CPU) );

//  fht_init();
  tx_init();
  display_channel = DISPLAY_USB;

#ifdef HAS_RF_ROUTER
  rf_router_init();
#endif

  LED_OFF();
  
  sei();
  
  for(;;) {
    uart_task();
    RfAnalyze_Task();
    Minute_Task();
#ifdef HAS_FASTRF
    if(fastrf_on)
      FastRF_Task();
#endif
#ifdef HAS_RF_ROUTER
    if(rf_router_status != RF_ROUTER_DISABLED)
      rf_router_task();
#endif
  }

}
