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

#if 0
void
memtest(char *in)
{
  uint8_t hb;

  if(in[1] == 'r') {
    fromhex(in+2, &hb, 1);
    dumpmem((uint8_t *)(hb<<8), 0xff);

  } else {
    fromhex(in+2, &hb, 1);
    uint8_t *addr = (uint8_t *)(hb<<8);

    for(uint8_t i = 0; i < 16; i++) {
      for(uint8_t j = 0; j < 16; j++)
        addr[j] = hb+j;
      DH((uint16_t)addr, 4); DC(' '); DH2(hb); DNL();
      addr += 16;
    }
  }
}
  { 'M', memtest },
#endif

PROGMEM t_fntab fntab[] = {

  { 'B', prepare_boot },
  { 'C', ccreg },
  { 'F', fs20send },
#ifdef HAS_RAWSEND
  { 'G', rawsend },
#endif
  { 'R', read_eeprom },
  { 'T', fhtsend },
  { 'W', write_eeprom },
  { 'V', version },
  { 'X', set_txreport },

  { 'e', eeprom_factory_reset },
#ifdef HAS_FASTRF
  { 'f', fastrf },
#endif
  { 'l', ledfunc },
  { 'q', tcplink_close },
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

#if defined(__AVR_AT90USB1286__)
#define jump_to_bootloader ((void(*)(void))0xf000)
#elif defined(__AVR_AT90USB646__)
#define jump_to_bootloader ((void(*)(void))0x7800) // BOOTSZ0=1,BOOTSZ1=0
#else
#define jump_to_bootloader ((void(*)(void))0x1800)
#endif

  jump_to_bootloader();
}

#ifdef HAS_XRAM

void init_memory_mapped (void) __attribute__ ((naked)) __attribute__ ((section (".init1")));


/*
 * Setup the External Memory Interface early during device initialization
 *   
*/
void
init_memory_mapped(void)
{

  DDRF  |= _BV( PF1 );
  PORTF &= ~_BV( PF1 ); // set A16 low

  // check EXTMEMOPTS in makefile for mem usage!
  /* enable external memory mapped interface */
  PORTA = 0xff;
  PORTC = 0xff;
  XMCRA = _BV(SRE); // | _BV( SRW10 );
  XMCRB = 0;
}

#endif

int
main(void)
{

  // reset Ethernet
  DDRD   |= _BV( PD6 );
  PORTD  &= ~_BV( PD6 );

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

#ifdef HAS_TOSC
  // Init RTC in processor located in Timer2 using external 32kHz crystal
  ASSR   |= _BV( AS2 );
  TCCR2B |= _BV( CS20) | _BV( CS21) | _BV( CS22); // prescaler 1024, gives 32 ticks per sec
#endif

  clock_prescale_set(clock_div_1);         // Disable Clock Division

  MCUSR &= ~(1 << WDRF);                   // Enable the watchdog
  wdt_enable(WDTO_2S); 

  led_init();

  rb_init(USB_Tx_Buffer, CDC_TX_EPSIZE);
  rb_init(USB_Rx_Buffer, CDC_RX_EPSIZE);
  USB_Init();
  tty_init();
  ethernet_init();

  fht_init();
  tx_init();

  output_enabled = OUTPUT_USB;

  LED_OFF();

  for(;;) {
    USB_USBTask();
    CDC_Task();
    RfAnalyze_Task();
    Minute_Task();
    if(fastrf_on)
      FastRF_Task();
    Ethernet_Task();
  }
}
