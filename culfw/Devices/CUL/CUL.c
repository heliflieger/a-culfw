/* Copyright Rudolf Koenig, 2008.
   Released under the GPL Licence, Version 2
   Inpired by the MyUSB USBtoSerial demo, Copyright (C) Dean Camera, 2008.
*/

#include <Drivers/USB/HighLevel/../LowLevel/LowLevel.h>  // for USB_Init
#include <Drivers/USB/HighLevel/USBTask.h>  // for USB_USBTask
#include <avr/interrupt.h>              // for cli
#include <avr/io.h>                     // for _BV, bit_is_set
#include <avr/pgmspace.h>               // for PROGMEM
#include <avr/wdt.h>                    // for WDTO_2S, wdt_enable

#include "board.h"                      // for HAS_ASKSIN, HAS_KOPP_FC, etc
#include "cc1100.h"                     // for ccreg, ccsetpa
#include "cdc.h"                        // for CDC_Task
#include "clock.h"                      // for Minute_Task, gettime
#include "display.h"                    // for DISPLAY_RFROUTER, etc
#include "fastrf.h"                     // for FastRF_Task, fastrf_func
#include "fband.h"                      // for checkFrequency
#include "fht.h"                        // for fht_init, fhtsend
#include "fncollection.h"               // for EE_REQBL, etc
#include "led.h"                        // for LED_OFF, led_init
#include "rf_receive.h"                 // for RfAnalyze_Task, etc
#include "rf_router.h"                  // for rf_router_func, etc
#include "rf_send.h"                    // for em_send, fs20send, hm_send, etc
#include "spi.h"                        // for spi_init
#include "ttydata.h"                    // for analyze_ttydata, etc

#ifdef HAS_MEMFN
#include "memory.h"                     // for getfreemem
#endif
#ifdef HAS_ASKSIN
#include "rf_asksin.h"                  // for asksin_func, rf_asksin_task
#endif
#ifdef HAS_MORITZ
#include "rf_moritz.h"                  // for moritz_func, rf_moritz_task
#endif
#ifdef HAS_RWE
#include "rf_rwe.h"                     // for rf_rwe_task, rwe_func
#endif
#ifdef HAS_RFNATIVE
#include "rf_native.h"                  // for native_func, native_task
#endif
#ifdef HAS_INTERTECHNO
#include "intertechno.h"                // for it_func
#endif
#ifdef HAS_SOMFY_RTS
#include "somfy_rts.h"                  // for somfy_rts_func
#endif
#ifdef HAS_MBUS
#include "rf_mbus.h"                    // for rf_mbus_func, rf_mbus_task
#endif
#ifdef HAS_KOPP_FC
#include "kopp-fc.h"                    // for kopp_fc_func, kopp_fc_task
#endif
#ifdef HAS_BELFOX
#include "belfox.h"                     // for send_belfox
#endif
#ifdef HAS_ZWAVE
#include "rf_zwave.h"                   // for rf_zwave_task, zwave_func
#endif

const PROGMEM t_fntab fntab[] = {

#ifdef HAS_ASKSIN
  { 'A', asksin_func },
#endif
  { 'B', prepare_boot },
#ifdef HAS_MBUS
  { 'b', rf_mbus_func },
#endif
  { 'C', ccreg },
#ifdef HAS_RWE
  { 'E', rwe_func },
#endif
  { 'e', eeprom_factory_reset },
  { 'F', fs20send },
#ifdef HAS_FASTRF
  { 'f', fastrf_func },
#endif
#ifdef HAS_RAWSEND
  { 'G', rawsend },
#endif
#ifdef HAS_HOERMANN_SEND
  { 'h', hm_send },
#endif
#ifdef HAS_INTERTECHNO
  { 'i', it_func },
#endif
#ifdef HAS_RAWSEND
  { 'K', ks_send },
#endif
#ifdef HAS_KOPP_FC
  { 'k', kopp_fc_func },
#endif
#ifdef HAS_BELFOX
  { 'L', send_belfox },
#endif
  { 'l', ledfunc },
#ifdef HAS_RAWSEND
  { 'M', em_send },
#endif
#ifdef HAS_MEMFN
  { 'm', getfreemem },
#endif
#ifdef HAS_RFNATIVE
  { 'N', native_func },
#endif
  { 'R', read_eeprom },
  { 'T', fhtsend },
  { 't', gettime },
#ifdef HAS_UNIROLL
  { 'U', ur_send },
#endif
#ifdef HAS_RF_ROUTER
  { 'u', rf_router_func },
#endif
  { 'V', version },
  { 'W', write_eeprom },
  { 'X', set_txreport },
  { 'x', ccsetpa },
#ifdef HAS_SOMFY_RTS
  { 'Y', somfy_rts_func },
#endif
#ifdef HAS_MORITZ
  { 'Z', moritz_func },
#endif
#ifdef HAS_ZWAVE
  { 'z', zwave_func },
#endif
  { 0, 0 },
};


void static
start_bootloader(void)
{
  cli();

  /* move interrupt vectors to bootloader section and jump to bootloader */
  MCUCR = _BV(IVCE);
  MCUCR = _BV(IVSEL);

#if defined(CUL_V3) || defined(CUL_V4)
#  define jump_to_bootloader ((void(*)(void))0x3800)
#endif
#if defined(CUL_V2)
#  define jump_to_bootloader ((void(*)(void))0x1800)
#endif
  jump_to_bootloader();
}

int
main(void)
{
  wdt_enable(WDTO_2S);
  clock_prescale_set(clock_div_1);

  MARK433_PORT |= _BV( MARK433_BIT ); // Pull 433MHz marker
  MARK915_PORT |= _BV( MARK915_BIT ); // Pull 915MHz marker

  // if we had been restarted by watchdog check the REQ BootLoader byte in the
  // EEPROM ...
  if(bit_is_set(MCUSR,WDRF) && erb(EE_REQBL)) {
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

  MCUSR &= ~(1 << WDRF);                   // Enable the watchdog

  led_init();
  spi_init();
  eeprom_init();
  USB_Init();
  fht_init();
  tx_init();
  input_handle_func = analyze_ttydata;
#ifdef HAS_RF_ROUTER
  rf_router_init();
  display_channel = (DISPLAY_USB|DISPLAY_RFROUTER);
#else
  display_channel = DISPLAY_USB;
#endif

  checkFrequency(); 
  LED_OFF();

  for(;;) {
    USB_USBTask();
    CDC_Task();
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
