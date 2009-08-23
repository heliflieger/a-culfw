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
#include <util/delay.h>

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
#include "memory.h"

#include "global-conf.h"
#include "uip_arp.h"
#include "drivers/interfaces/network.h"
#include "drivers/enc28j60/enc28j60.h"
#include "timer.h"

void Ethernet_Task(void);
#define BUF ((struct uip_eth_hdr *)&uip_buf[0])
struct timer periodic_timer, arp_timer;

static uint8_t usbtask_enabled;

void
checkbits(char *in)
{
  uint8_t  SigReadAddress     = 0x0E;
  for (uint8_t SerialCharNum = 0; SerialCharNum < 20; SerialCharNum++)
  {
    uint8_t SerialByte = boot_signature_byte_get(SigReadAddress);
    if(SerialCharNum & 0x01) {
      SerialByte >>= 4;
      SigReadAddress++;
    }
    DH2(SerialByte);
  }
  DNL();
  DH2(boot_lock_fuse_bits_get(GET_LOW_FUSE_BITS));
  DH2(boot_lock_fuse_bits_get(GET_HIGH_FUSE_BITS));
  DH2(boot_lock_fuse_bits_get(GET_EXTENDED_FUSE_BITS));
  DH2(boot_lock_fuse_bits_get(GET_LOCK_BITS));
  DNL();
}

PROGMEM t_fntab fntab[] = {

  { 'B', prepare_boot },
  { 'C', ccreg },
  { 'F', fs20send },
#ifdef HAS_RAWSEND
  { 'G', rawsend },
#endif
  { 'M', testmem },
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
  { 'm', getfreemem },
  { 'q', tcplink_close },
  { 't', gettime },
  { 'x', ccsetpa },

  { 'z', checkbits },

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
 *  * Setup the External Memory Interface early during device initialization
 *   
*/

void init_memory_mapped(void) {    

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

void
start_networking (void)
{
  // reset Ethernet
  DDRD   |= _BV( PD6 );
  PORTD  &= ~_BV( PD6 );

  my_delay_ms( 200 );

  // unreset Ethernet
  PORTD |= _BV( PD6 );

  my_delay_ms( 200 );

  network_init();
  
  uip_ipaddr_t ipaddr;
  
  // setup two periodic timers
  timer_set(&periodic_timer, CLOCK_SECOND / 2);
  timer_set(&arp_timer, CLOCK_SECOND * 10);
  
  uip_init();
  
  struct uip_eth_addr mac = {
    { UIP_ETHADDR0, UIP_ETHADDR1, UIP_ETHADDR2,
      UIP_ETHADDR3, UIP_ETHADDR4, UIP_ETHADDR5 }
  };
  
  uip_setethaddr(mac);

#ifdef __DHCPC_H__
  dhcpc_init(&mac, 6);
#else
  uip_ipaddr(ipaddr, 192,168,178,9);
  uip_sethostaddr(ipaddr);
  uip_ipaddr(ipaddr, 192,168,178,1);
  uip_setdraddr(ipaddr);
  uip_ipaddr(ipaddr, 255,255,255,0);
  uip_setnetmask(ipaddr);

//  simple_httpd_init();
//  telnetd_init();
  tcplink_init();
  
#endif 
  
}


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

  SetSystemClockPrescaler(0);              // Disable Clock Division

  MCUSR &= ~(1 << WDRF);                   // Enable the watchdog
  wdt_enable(WDTO_2S); 

  led_init();

  rb_init(USB_Tx_Buffer, CDC_TX_EPSIZE);
  rb_init(USB_Rx_Buffer, CDC_RX_EPSIZE);

  start_networking();

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
    Ethernet_Task();
  }

}

void
Ethernet_Task()
{
  int i;
  
  uip_len = network_read();
  
  if(uip_len > 0) {
       if(BUF->type == htons(UIP_ETHTYPE_IP)){
            uip_arp_ipin();
            uip_input();
            if(uip_len > 0) {
                 uip_arp_out();
                 network_send();
            }
       } else if(BUF->type == htons(UIP_ETHTYPE_ARP)){
            uip_arp_arpin();
            if(uip_len > 0){
                 network_send();
            }
       }
       
  } else if(timer_expired(&periodic_timer)) {
       timer_reset(&periodic_timer);
       
       for(i = 0; i < UIP_CONNS; i++) {
            uip_periodic(i);
            if(uip_len > 0) {
                 uip_arp_out();
                 network_send();
            }
       }
       
#if UIP_UDP
       for(i = 0; i < UIP_UDP_CONNS; i++) {
            uip_udp_periodic(i);
            if(uip_len > 0) {
                 uip_arp_out();
                 network_send();
            }
       }
#endif
       
       if(timer_expired(&arp_timer)) {
            timer_reset(&arp_timer);
            uip_arp_timer();
            
       }
  }

}

#ifdef __DHCPC_H__
void dhcpc_configured(const struct dhcpc_state *s)
{
  uip_sethostaddr(s->ipaddr);
  uip_setnetmask(s->netmask);
  uip_setdraddr(s->default_router);
  //resolv_conf(s->dnsaddr);
}
#endif /* __DHCPC_H__ */
