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
#include <util/delay.h>

#include <string.h>

#include <Drivers/USB/USB.h>     // USB Functionality

#include "spi.h"
#include "cc1100.h"
#include "cdc.h"
#include "clock.h"
#include "delay.h"
#include "display.h"
#include "fncollection.h"
#include "led.h"
#include "ringbuffer.h"
#include "rf_receive.h"
#include "rf_send.h"
#include "ttydata.h"
#include "fht.h"
#include "fastrf.h"
#include "memory.h"
#include "ethernet.h"
#include "tcplink.h"
#include "ntp.h"
#include "cctemp.h"
#include "rf_router.h"

#ifdef HAS_FS
#include "fswrapper.h"
#include "log.h"

df_chip_t df;
#endif

#ifdef HAS_ASKSIN
#include "rf_asksin.h"
#endif

const PROGMEM t_fntab fntab[] = {

  { 'M', testmem },
  { 'm', getfreemem },

  { 'B', prepare_boot },
  { 'C', ccreg },
  { 'E', eth_func },
  { 'F', fs20send },
  { 'M', em_send },
  { 'G', rawsend },
  { 'R', read_eeprom },
  { 'T', fhtsend },
  { 'W', write_eeprom },
  { 'V', version },
  { 'X', set_txreport },

  { 'c', ntp_func },
  { 'e', eeprom_factory_reset },
//  { 'h', cctemp_func },       // HU: hömérsék :)
  { 'f', fastrf_func },
  { 'l', ledfunc },
  { 'q', tcplink_close },
  { 't', gettime },
  { 'u', rf_router_func },
  { 'x', ccsetpa },
#ifdef HAS_FS
  { 'r', read_file },
  { 'w', write_file },
#endif
#ifdef HAS_ASKSIN
  { 'A', asksin_func },
#endif

  { 0, 0 },
};


void
start_bootloader(void)
{
  cli();

  /* move interrupt vectors to bootloader section and jump to bootloader */
  MCUCR = _BV(IVCE);
  MCUCR = _BV(IVSEL);

#if (defined(__AVR_AT90USB1286__) || defined(__AVR_AT90USB1287__))
#  define jump_to_bootloader ((void(*)(void))0xf000)
#elif (defined(__AVR_AT90USB646__) || defined(__AVR_AT90USB647__))
#  define jump_to_bootloader ((void(*)(void))0x7800) // BOOTSZ0=1,BOOTSZ1=0
#else
#  define jump_to_bootloader ((void(*)(void))0x1800)
#endif

  jump_to_bootloader();
}

#ifdef HAS_XRAM

/*
 * Setup the External Memory Interface early during device initialization
 *   
*/

void init_memory_mapped (void) __attribute__ ((naked))
                               __attribute__ ((section (".init1")));

void
init_memory_mapped(void)
{
  // check EXTMEMOPTS in makefile for mem usage!
  /* enable external memory mapped interface */

#ifdef CUN_V10
  DDRF  |= _BV( PF1 );
  PORTF &= ~_BV( PF1 ); // set A16 low
#endif

  PORTA = 0xff;
  PORTC = 0xff;
  XMCRA = _BV(SRE); // | _BV( SRW10 );
  XMCRB = 0;
}
#endif

int
main(void)
{
  //ewb((uint8_t*)0x80, erb((uint8_t*)0x80)+1);
  wdt_enable(WDTO_2S);                     // Avoid an early reboot
  clock_prescale_set(clock_div_1);         // Disable Clock Division:1->8MHz
#ifdef HAS_XRAM
  init_memory_mapped(); // First initialize the RAM
#endif

  // if we had been restarted by watchdog check the REQ BootLoader byte in the
  // EEPROM
  if(bit_is_set(MCUSR,WDRF) && erb(EE_REQBL)) {
    ewb( EE_REQBL, 0 ); // clear flag
    start_bootloader();
  }
  while(tx_report);                     // reboot if the bss is not initialized

  // Setup the timers. Are needed for watchdog-reset
  OCR0A  = 249;                            // Timer0: 0.008s = 8MHz/256/250
  TCCR0B = _BV(CS02);       
  TCCR0A = _BV(WGM01);
  TIMSK0 = _BV(OCIE0A);

  TCCR1A = 0;
  TCCR1B = _BV(CS11) | _BV(WGM12);         // Timer1: 1us = 8MHz/8

  MCUSR &= ~(1 << WDRF);                   // Enable the watchdog

  led_init();                              // So we can debug 
  spi_init();
  eeprom_init();
  USB_Init();
  fht_init();
  tx_init();
  ethernet_init();
#ifdef HAS_FS
  df_init(&df);
  fs_init(&fs, df, 0);          // needs df_init
  log_init();                   // needs fs_init & rtc_init
  log_enabled = erb(EE_LOGENABLED);
#endif
  input_handle_func = analyze_ttydata;
  display_channel = DISPLAY_USB;

  LED_OFF();

  for(;;) {
    USB_USBTask();
    CDC_Task();
    RfAnalyze_Task();
    Minute_Task();
    FastRF_Task();
    rf_router_task();
#ifdef HAS_ASKSIN
    rf_asksin_task();
#endif
#ifdef HAS_ETHERNET
    Ethernet_Task();
#endif
  }
}
