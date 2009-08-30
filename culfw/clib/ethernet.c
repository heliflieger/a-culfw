#include <avr/pgmspace.h>
#include <avr/boot.h>
#include "ethernet.h"
#include "fncollection.h"
#include "stringfunc.h"
#include "timer.h"
#include "display.h"
#include "delay.h"

#include "global-conf.h"
#include "uip_arp.h"
#include "drivers/interfaces/network.h"
#include "drivers/enc28j60/enc28j60.h"
#include "apps/dhcpc/dhcpc.h"
#include "delay.h"

#define BUF ((struct uip_eth_hdr *)uip_buf)
struct timer periodic_timer, arp_timer;

uint8_t
bsbg(uint8_t a)
{
  return boot_signature_byte_get(a);
}

void
ethernet_reset(void)
{
  char buf[21];

  buf[1] = 'i';
  buf[2] = 'd'; strcpy_P(buf+3, PSTR("1"));             write_eeprom(buf);
  buf[2] = 'a'; strcpy_P(buf+3, PSTR("192.168.0.244")); write_eeprom(buf);
  buf[2] = 'n'; strcpy_P(buf+3, PSTR("255.255.255.0")); write_eeprom(buf);
  buf[2] = 'g'; strcpy_P(buf+3, PSTR("192.168.0.1"));   write_eeprom(buf);
  buf[2] = 'p'; strcpy_P(buf+3, PSTR("2323"));          write_eeprom(buf);
  buf[2] = 'm'; strcpy_P(buf+3, PSTR("000425"));        // Atmel MAC Range

  // Generate a "unique" MAC address from the unique serial number
  tohex(bsbg(0x0e)+bsbg(0x10), (uint8_t*)buf+9);
  tohex(bsbg(0x12)+bsbg(0x14), (uint8_t*)buf+11);
  tohex(bsbg(0x16)+bsbg(0x18), (uint8_t*)buf+13);
  buf[15] = 0;
  write_eeprom(buf);
}

void
dumppkt(void)
{
  uint8_t *a = uip_buf;

  output_enabled &= ~OUTPUT_TCP;
  DC(' '); DC('d'); DC(':');
  for(uint8_t i = 0; i < sizeof(struct uip_eth_addr); i++)
    DH2(*a++);
  DC(' '); DC('s'); DC(':');
  for(uint8_t i = 0; i < sizeof(struct uip_eth_addr); i++)
    DH2(*a++);

  DC(' '); DC('t'); DC(':');
  DH2(*a++);
  DH2(*a++);
  DNL();

  dumpmem(a, uip_len - sizeof(struct uip_eth_hdr));
  output_enabled |= OUTPUT_TCP;
}

void
Ethernet_Task(void)
{
  int i;
  
  uip_len = network_read();

  if(uip_len > 0) {
       //DC('e'); DH(uip_len,4); dumppkt(); DNL();
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

static void                             // EEPROM Read IP
erip(uip_ipaddr_t ip, uint8_t *addr)
{
  uip_ipaddr(ip, erb(addr), erb(addr+1), erb(addr+2), erb(addr+3));
}

static void                             // EEPROM Write IP
ewip(const u16_t ip[2], uint8_t *addr)
{
  uint16_t ip0 = HTONS(ip[0]);
  uint16_t ip1 = HTONS(ip[1]);
  ewb(addr+0, ip0>>8);
  ewb(addr+1, ip0&0xff);
  ewb(addr+2, ip1>>8);
  ewb(addr+3, ip1&0xff);
}

void
dhcpc_configured(const struct dhcpc_state *s)
{
  ewip(s->ipaddr, EE_IP4_ADDR);            uip_sethostaddr(s->ipaddr);
  ewip(s->default_router, EE_IP4_GATEWAY); uip_setdraddr(s->default_router);
  ewip(s->netmask, EE_IP4_NETMASK);        uip_setnetmask(s->netmask);
  //resolv_conf(s->dnsaddr);
  enc28j60PhyWrite(PHLCON,0x476);// LED A: Link Status  LED B: TX/RX
}


void
ethernet_init(void)
{
  // reset Ethernet
  DDRD   |= _BV( PD6 );
  PORTD  &= ~_BV( PD6 );

  my_delay_ms( 200 );

  // unreset Ethernet
  PORTD |= _BV( PD6 );

  my_delay_ms( 200 );
  network_init();
  
  // setup two periodic timers
  timer_set(&periodic_timer, CLOCK_SECOND / 2);
  timer_set(&arp_timer, CLOCK_SECOND * 10);
  
  uip_init();
  
  static struct uip_eth_addr mac;       // static for dhcpc
  mac.addr[0] = erb(EE_MAC_ADDR+0);
  mac.addr[1] = erb(EE_MAC_ADDR+1);
  mac.addr[2] = erb(EE_MAC_ADDR+2);
  mac.addr[3] = erb(EE_MAC_ADDR+3);
  mac.addr[4] = erb(EE_MAC_ADDR+4);
  mac.addr[5] = erb(EE_MAC_ADDR+5);
  uip_setethaddr(mac);
  network_set_MAC(mac.addr);

  if(erb(EE_USE_DHCP)) {
    enc28j60PhyWrite(PHLCON,0x4A6);// LED A: Link Status  LED B: Blink slow
    dhcpc_init(mac.addr, 6);

  } else {
    uip_ipaddr_t ipaddr;
    erip(ipaddr, EE_IP4_ADDR);    uip_sethostaddr(ipaddr);
    erip(ipaddr, EE_IP4_GATEWAY); uip_setdraddr(ipaddr);
    erip(ipaddr, EE_IP4_NETMASK); uip_setnetmask(ipaddr);
    enc28j60PhyWrite(PHLCON,0x476);// LED A: Link Status  LED B: TX/RX

  }
    
  tcplink_init();
  
}

