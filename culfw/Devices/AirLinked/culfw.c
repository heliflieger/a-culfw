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
#ifdef HAS_UART
#include "serial.h"
#endif
#include "fncollection.h"
#include "led.h"
#include "ringbuffer.h"
#include "rf_receive.h"
#include "rf_send.h"
#include "ttydata.h"
#include "fht.h"
#include "fastrf.h"
#include "rf_router.h"
#include "memory.h"

#ifdef HAS_INTERTECHNO
#include "intertechno.h"
#endif

#ifdef HAS_ASKSIN
#include "rf_asksin.h"
#endif

#ifdef HAS_DOGM
#include "dogm16x.h"
#endif

#if defined (HAS_IRRX) || defined (HAS_IRTX)
#include "ir.h"
#endif

#ifdef HAS_OWN_ADDRESS
#include "myaddress.h"
#endif

const PROGMEM t_fntab fntab[] = {

  { 'm', getfreemem },

  { 'B', prepare_boot },
  { 'C', ccreg },
  { 'F', fs20send },
#ifdef HAS_INTERTECHNO
  { 'i', it_func },
#endif
#ifdef HAS_ASKSIN
  { 'A', asksin_func },
#endif
#ifdef HAS_OWN_ADDRESS
  { 'a', myaddress_func },
#endif
#if defined (HAS_IRRX) || defined (HAS_IRTX)
  { 'I', ir_func },
#endif
#ifdef HAS_DOGM
  { 'D', dogm_func },
#endif
#ifdef HAS_RAWSEND
  { 'G', rawsend },
  { 'M', em_send },
//  { 'S', esa_send },
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

void culfw_init(void) {
  wdt_disable();

  led_init();
  LED_ON();

  spi_init();

//  eeprom_factory_reset("xx");
  eeprom_init();

//  led_mode = 2;

  // if we had been restarted by watchdog check the REQ BootLoader byte in the
  // EEPROM ...
//  if(bit_is_set(MCUSR,WDRF) && eeprom_read_byte(EE_REQBL)) {
//    eeprom_write_byte( EE_REQBL, 0 ); // clear flag
//    start_bootloader();
//  }

  // Setup the timers. Are needed for watchdog-reset
#if defined (HAS_IRRX) || defined (HAS_IRTX)
  ir_init();
  // IR uses highspeed TIMER0 for sampling 
  OCR0A  = 1;                              // Timer0: 0.008s = 8MHz/256/2   == 15625Hz
#else
  OCR0A  = 249;                            // Timer0: 0.008s = 8MHz/256/250 == 125Hz
#endif
  TCCR0B = _BV(CS02);
  TCCR0A = _BV(WGM01);
  TIMSK0 = _BV(OCIE0A);

  TCCR1A = 0;
  TCCR1B = _BV(CS11) | _BV(WGM12);         // Timer1: 1us = 8MHz/8

  clock_prescale_set(clock_div_1);

  MCUSR &= ~(1 << WDRF);                   // Enable the watchdog
  wdt_enable(WDTO_2S);

#ifdef HAS_UART
  uart_init( UART_BAUD_SELECT_DOUBLE_SPEED(UART_BAUD_RATE,F_CPU) );
#endif

#ifdef HAS_DOGM
  dogm_init();
#endif

#ifdef HAS_OWN_ADDRESS
  myaddress_init();
#endif

  fht_init();
  tx_init();
  input_handle_func = analyze_ttydata;

  display_channel = DISPLAY_USB;

#ifdef HAS_RF_ROUTER
  rf_router_init();
  display_channel |= DISPLAY_RFROUTER;
#endif

#ifdef HAS_DOGM
  display_channel |= DISPLAY_DOGM;
#endif

  LED_OFF();

  sei();
}

void culfw_loop(void) {
#ifdef HAS_UART
    uart_task();
#endif
    RfAnalyze_Task();
    Minute_Task();
#ifdef HAS_FASTRF
    FastRF_Task();
#endif
#ifdef HAS_RF_ROUTER
    rf_router_task();
#endif
#ifdef HAS_ASKSIN
    rf_asksin_task();
#endif
#if defined (HAS_IRRX) || defined (HAS_IRTX)
    ir_task();
#endif

}
