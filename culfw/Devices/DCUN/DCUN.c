/* Copyright Rudolf Koenig, 2008.
   Released under the GPL Licence, Version 2
   Inpired by the MyUSB USBtoSerial demo, Copyright (C) Dean Camera, 2008.
*/

#include <avr/io.h>
#include <avr/boot.h>
#include <avr/eeprom.h>
#include <string.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>

#include <MyUSB/Drivers/USB/USB.h>     // USB Functionality
#include <MyUSB/Scheduler/Scheduler.h> // Simple scheduler for task management
#include <Drivers/AT90USB162/SPI.h>    // SPI drivers

#include "cc1100.h"
#include "cdc.h"
#include "clock.h"
#include "delay.h"
#include "display.h"
#include "fncollection.h"
#include "led.h"
#include "ringbuffer.h"
#include "transceiver.h"
#include "ttydata.h"

#include "global-conf.h"
#include "uip_arp.h"
#include "network.h"
#include "enc28j60.h"

#include "timer.h"

TASK(Ethernet_Task);

/* Scheduler Task List */
TASK_LIST
{
  { Task: USB_USBTask  , TaskStatus: TASK_STOP },
  { Task: CDC_Task     , TaskStatus: TASK_STOP },
  { Task: RfAnalyze_Task,TaskStatus: TASK_RUN },
//  { Task: Minute_Task,   TaskStatus: TASK_STOP },
  { Task: Ethernet_Task,   TaskStatus: TASK_RUN },
};

t_fntab fntab[] = {

  { 'b', prepare_b },
  { 'B', prepare_B },
  { 'C', ccreg },
  { 'F', fs20send },
  { 'l', ledfunc },
  { 'R', read_eeprom },
  { 'T', fhtsend },
  { 't', gettime },
  { 'V', version },
  { 'W', write_eeprom },
  { 'X', set_txreport },
  { 0, 0 },
};


#if defined(__AVR_AT90USB1286__)
#define jump_to_bootloader ((void(*)(void))0xf000)
#else
#define jump_to_bootloader ((void(*)(void))0x3000)
#endif

void
start_bootloader(void)
{
  cli();
  
  /* move interrupt vectors to bootloader section and jump to bootloader */
  MCUCR = _BV(IVCE);
  MCUCR = _BV(IVSEL);

  jump_to_bootloader();
}

EVENT_HANDLER(USB_Connect) {
     Scheduler_SetTaskMode(USB_USBTask, TASK_RUN);
}

EVENT_HANDLER(USB_Disconnect) {
     Scheduler_SetTaskMode(USB_USBTask, TASK_STOP);
     Scheduler_SetTaskMode(CDC_Task,    TASK_STOP);
     ccStrobe(CC1100_SIDLE);
}

#define BUF ((struct uip_eth_hdr *)&uip_buf[0])
struct timer periodic_timer, arp_timer;

int main(void) {

  // if we had been restarted by watchdog check the REQ BootLoader byte in the
  // EEPROM ...
  if (bit_is_set(MCUSR,WDRF) && eeprom_read_byte(EE_REQBL)) {
       eeprom_write_byte( EE_REQBL, 0 ); // clear flag
       start_bootloader();
  }

  MCUSR &= ~(1 << WDRF);

  // reset Ethernet
  DDRB   |= _BV( PB4 );
  PORTB  &= ~_BV( PB4 );

  DDRE  |= _BV( PE1 );
  PORTE |= _BV( PE1 );

//  wdt_enable(WDTO_2S); 

  SetSystemClockPrescaler(0);                   // Disable Clock Division

  led_init();
  SPI_MasterInit(DATA_ORDER_MSB);
  
  rb_init(Tx_Buffer, TX_SIZE);
  credit_10ms = MAX_CREDIT/2;

  minute = 0;
  OCR0A  = 249;  // Timer0: 0.008s (1/125sec) = 8MHz/256/250  (?)
  TCCR0B = _BV(CS02);       
  TCCR0A = _BV(WGM01);
  TIMSK0 = _BV(OCIE0A);


  // Network init
  PORTB |= _BV( PB4 ); // unreset
  network_init();

  uip_ipaddr_t ipaddr;

  timer_set(&periodic_timer, CLOCK_SECOND / 2);
  timer_set(&arp_timer, CLOCK_SECOND * 10);

  uip_init();

  struct uip_eth_addr mac = {UIP_ETHADDR0, UIP_ETHADDR1, UIP_ETHADDR2, UIP_ETHADDR3, UIP_ETHADDR4, UIP_ETHADDR5};

  uip_setethaddr(mac);
  simple_httpd_init();

#ifdef __DHCPC_H__
  dhcpc_init(&mac, 6);
#else
  uip_ipaddr(ipaddr, 10,10,12,123);
  uip_sethostaddr(ipaddr);
  uip_ipaddr(ipaddr, 10,10,12,1);
  uip_setdraddr(ipaddr);
  uip_ipaddr(ipaddr, 255,255,255,0);
  uip_setnetmask(ipaddr);
#endif /*__DHCPC_H__*/

  Scheduler_Init();                            

  USB_Init();

  Scheduler_Start();                            // Won't return
}

TASK(Ethernet_Task) {
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
	  }else if(BUF->type == htons(UIP_ETHTYPE_ARP)){
	       uip_arp_arpin();
	       if(uip_len > 0){
		    network_send();
	       }
	  }
	  
     }else if(timer_expired(&periodic_timer)) {
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
#endif /* UIP_UDP */
	  
	  if(timer_expired(&arp_timer)) {
	       PORTE  ^= _BV( PE1 );
	       timer_reset(&arp_timer);
	       uip_arp_timer();
	  }
     }
     
}

