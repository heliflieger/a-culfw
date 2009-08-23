/* Copyright Rudolf Koenig, 2008.
   Released under the GPL Licence, Version 2
   Inpired by the MyUSB USBtoSerial demo, Copyright (C) Dean Camera, 2008.
*/

#include <avr/boot.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>

#include <string.h>

#include <MyUSB/Drivers/USB/USB.h>     // USB Functionality

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

static uint8_t usbtask_enabled;

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
  { 'f', fastrf },
#endif
  { 'l', ledfunc },
  { 't', gettime },
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

EVENT_HANDLER(USB_Connect)
{
  usbtask_enabled = 1;
}

EVENT_HANDLER(USB_Disconnect)
{
  usbtask_enabled = 0;
  cdctask_enabled = 0;
  ccStrobe(CC1100_SIDLE);
}

int
main(void)
{

  PORTB |= _BV( PB6 ); // Pull 433MHz marker

  spi_init();
  eeprom_init();

  // if we had been restarted by watchdog check the REQ BootLoader byte in the
  // EEPROM ...
  if(bit_is_set(MCUSR,WDRF) && eeprom_read_byte(EE_REQBL)) {
    eeprom_write_byte( EE_REQBL, 0 ); // clear flag
    start_bootloader();
  }

  // Setup the timers. Are needed for watchdog-reset
  OCR0A  = 249;                            // Timer0: 0.008s = 8MHz/256/250
  TCCR0B = _BV(CS02);       
  TCCR0A = _BV(WGM01);
  TIMSK0 = _BV(OCIE0A);

  TCCR1A = 0;
  TCCR1B = _BV(CS11) | _BV(WGM12);         // Timer1: 1us = 8MHz/8

  SetSystemClockPrescaler(0);              // Disable Clock Division

  MCUSR &= ~(1 << WDRF);                   // Enable the watchdog
  wdt_enable(WDTO_2S); 

  led_init();
  rb_init(USB_Tx_Buffer, CDC_TX_EPSIZE);
  rb_init(USB_Rx_Buffer, CDC_RX_EPSIZE);
  USB_Init();
  tty_init();
  fht_init();
  tx_init();
  output_enabled = OUTPUT_USB;

  LED_OFF();

  for(;;) {
    if(usbtask_enabled)
      USB_USBTask();
    if(cdctask_enabled)
      CDC_Task();
    RfAnalyze_Task();
    Minute_Task();
    if(fastrf_on)
      FastRF_Task();
  }
}
