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

#include "board.h"

#include "fband.h"
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
#include "fastrf.h"
#include "rf_router.h"
#include "intertechno.h"
#include "fht.h"

#ifdef HAS_MEMFN
#include "memory.h"
#endif
#ifdef HAS_ASKSIN
#include "rf_asksin.h"
#endif
#ifdef HAS_MORITZ
#include "rf_moritz.h"
#endif
#ifdef HAS_RWE
#include "rf_rwe.h"
#endif
#ifdef HAS_RFNATIVE
#include "rf_native.h"
#endif
#ifdef HAS_INTERTECHNO
#include "intertechno.h"
#endif
#ifdef HAS_SOMFY_RTS
#include "somfy_rts.h"
#endif
#ifdef HAS_MBUS
#include "rf_mbus.h"
#endif
#ifdef HAS_KOPP_FC
#include "kopp-fc.h"
#endif
#ifdef HAS_BELFOX
#include "belfox.h"
#endif
#ifdef HAS_ZWAVE
#include "rf_zwave.h"
#endif

const PROGMEM t_fntab fntab[] = {
  { 'B', prepare_boot },
#ifdef HAS_MBUS
  { 'b', rf_mbus_func },
#endif
  { 'C', ccreg },
  { 'F', fs20send },
#ifdef HAS_INTERTECHNO
  { 'i', it_func },
#endif
#ifdef HAS_ASKSIN
  { 'A', asksin_func },
#endif
#ifdef HAS_MORITZ
  { 'Z', moritz_func },
#endif
#ifdef HAS_RFNATIVE
  { 'N', native_func },
#endif
#ifdef HAS_RWE
  { 'E', rwe_func },
#endif
#ifdef HAS_KOPP_FC
  { 'k', kopp_fc_func },
#endif
#ifdef HAS_RAWSEND
  { 'G', rawsend },
  { 'M', em_send },
  { 'K', ks_send },
#endif
#ifdef HAS_UNIROLL
  { 'U', ur_send },
#endif
#ifdef HAS_SOMFY_RTS
  { 'Y', somfy_rts_func },
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
#ifdef HAS_MEMFN
  { 'm', getfreemem },
#endif
#ifdef HAS_BELFOX
  { 'L', send_belfox },
#endif
  { 'l', ledfunc },
  { 't', gettime },
#ifdef HAS_RF_ROUTER
  { 'u', rf_router_func },
#endif
  { 'x', ccsetpa },
#ifdef HAS_ZWAVE
  { 'z', zwave_func },
#endif
  { 0, 0 },
};

int
main(void)
{
//  wdt_disable();

  clock_prescale_set(clock_div_1);

  MARK433_PORT |= _BV( MARK433_BIT ); // Pull 433MHz marker
  MARK915_PORT |= _BV( MARK915_BIT ); // Pull 915MHz marker

  led_init();

  spi_init();

  // Setup the timers. Are needed for watchdog-reset
  OCR0A  = 249;                            // Timer0: 0.008s = 8MHz/256/250 == 125Hz
  TCCR0B = _BV(CS02);
  TCCR0A = _BV(WGM01);
  TIMSK0 = _BV(OCIE0A);

  TCCR1A = 0;
  TCCR1B = _BV(CS11) | _BV(WGM12);         // Timer1: 1us = 8MHz/8

  //eeprom_factory_reset("xx");
  eeprom_init();

  MCUSR &= ~(1 << WDRF);                   // Enable the watchdog
  wdt_enable(WDTO_2S);

  //uart_init( UART_BAUD_SELECT_DOUBLE_SPEED(UART_BAUD_RATE,F_CPU) );
  uart_init( UART_BAUD_SELECT_DOUBLE_SPEED(UART_BAUD_RATE,F_CPU) );

  fht_init();
  tx_init();
  input_handle_func = analyze_ttydata;

  display_channel = DISPLAY_USB;

#ifdef HAS_RF_ROUTER
  rf_router_init();
  display_channel |= DISPLAY_RFROUTER;
#endif

  checkFrequency(); 
  sei();

  for(;;) {
    uart_task();
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
#ifdef HAS_MORITZ
    rf_moritz_task();
#endif
#ifdef HAS_RWE
    rf_rwe_task();
#endif
#ifdef HAS_RFNATIVE
    native_task();
#endif
#ifdef HAS_KOPP_FC
    kopp_fc_task();
#endif
#ifdef HAS_MBUS
    rf_mbus_task();
#endif
#ifdef HAS_ZWAVE
    rf_zwave_task();
#endif
  }

}
