#include <avr/pgmspace.h>
#include <avr/boot.h>
#include "board.h"
#include "ethernet.h"
#include "fncollection.h"
#include "stringfunc.h"
#include "timer.h"
#include "display.h"
#include "delay.h"
#include "ntp.h"
#include "mdns_sd.h"
#include "led.h"

#include "uip_arp.h"
#include "drivers/interfaces/network.h"
#include "apps/dhcpc/dhcpc.h"
#include "delay.h"

struct timer periodic_timer, arp_timer;
static struct uip_eth_addr mac;       // static for dhcpc
uint8_t eth_debug = 0;

static uint8_t dhcp_state;

static void set_eeprom_addr(void);
static void ip_initialized(void);

void
ethernet_init(void)
{

  // reset Ethernet
  ENC28J60_RESET_DDR  |= _BV( ENC28J60_RESET_BIT );
  ENC28J60_RESET_PORT &= ~_BV( ENC28J60_RESET_BIT );

  my_delay_ms( 200 );
  // unreset Ethernet
  ENC28J60_RESET_PORT |= _BV( ENC28J60_RESET_BIT );

  my_delay_ms( 200 );
  network_init();
  mac.addr[0] = erb(EE_MAC_ADDR+0);
  mac.addr[1] = erb(EE_MAC_ADDR+1);
  mac.addr[2] = erb(EE_MAC_ADDR+2);
  mac.addr[3] = erb(EE_MAC_ADDR+3);
  mac.addr[4] = erb(EE_MAC_ADDR+4);
  mac.addr[5] = erb(EE_MAC_ADDR+5);
  network_set_MAC(mac.addr);

  uip_setethaddr(mac);
  uip_init();
  ntp_conn = 0;

  // setup two periodic timers
  timer_set(&periodic_timer, CLOCK_SECOND / 4);
  timer_set(&arp_timer, CLOCK_SECOND * 10);

  if(erb(EE_USE_DHCP)) {
    network_set_led(0x4A6);// LED A: Link Status  LED B: Blink slow
    dhcpc_init(&mac);
    dhcp_state = PT_WAITING;

  } else {
    dhcp_state = PT_ENDED;
    set_eeprom_addr();

  }
}

void
ethernet_reset(void)
{
  char buf[21];
  uint16_t serial = 0;

  buf[1] = 'i';
  buf[2] = 'd'; strcpy_P(buf+3, PSTR("1"));             write_eeprom(buf);//DHCP
  buf[2] = 'a'; strcpy_P(buf+3, PSTR("192.168.0.244")); write_eeprom(buf);//IP
  buf[2] = 'n'; strcpy_P(buf+3, PSTR("255.255.255.0")); write_eeprom(buf);
  buf[2] = 'g'; strcpy_P(buf+3, PSTR("192.168.0.1"));   write_eeprom(buf);//GW
  buf[2] = 'p'; strcpy_P(buf+3, PSTR("2323"));          write_eeprom(buf);
  buf[2] = 'N'; strcpy_P(buf+3, PSTR("0.0.0.0"));       write_eeprom(buf);//==GW
  buf[2] = 'o'; strcpy_P(buf+3, PSTR("00"));            write_eeprom(buf);//GMT

#ifdef EE_DUDETTE_MAC
  // check for mac stored during manufacture
  uint8_t *ee = EE_DUDETTE_MAC;
  if (erb( ee++ ) == 0xa4)
    if (erb( ee++ ) == 0x50)
      if (erb( ee++ ) == 0x55) {
        buf[2] = 'm'; strcpy_P(buf+3, PSTR("A45055"));        // busware.de OUI range
        tohex(erb( ee++ ), (uint8_t*)buf+9);
        tohex(erb( ee++ ), (uint8_t*)buf+11);
        tohex(erb( ee++ ), (uint8_t*)buf+13);
        buf[15] = 0;
        write_eeprom(buf);
        return;
      } 
#endif

  // Generate a "unique" MAC address from the unique serial number
  buf[2] = 'm'; strcpy_P(buf+3, PSTR("A45055"));        // busware.de OUI range
#define bsbg boot_signature_byte_get

//  tohex(bsbg(0x0e)+bsbg(0x0f), (uint8_t*)buf+9);
//  tohex(bsbg(0x10)+bsbg(0x11), (uint8_t*)buf+11);
//  tohex(bsbg(0x12)+bsbg(0x13), (uint8_t*)buf+13);

  for (uint8_t i = 0x00; i < 0x20; i++) 
       serial += bsbg(i);

  tohex(0, (uint8_t*)buf+9);
  tohex((serial>>8) & 0xff, (uint8_t*)buf+11);
  tohex(serial & 0xff, (uint8_t*)buf+13);
  
  buf[15] = 0;
  write_eeprom(buf);
}

static void
display_mac(uint8_t *a)
{
  uint8_t cnt = 6;
  while(cnt--) {
    DH2(*a++);
    if(cnt)
      DC(':');
  }
}

static void
display_ip4(uint8_t *a)
{
  uint8_t cnt = 4;
  while(cnt--) {
    DU(*a++,1);
    if(cnt)
      DC('.');
  }
}

void
eth_func(char *in)
{
  if(in[1] == 'i') {
    ethernet_init();

  } else if(in[1] == 'c') {
    display_ip4((uint8_t *)uip_hostaddr); DC(' ');
    display_mac((uint8_t *)uip_ethaddr.addr);
    DNL();

  } else if(in[1] == 'd') {
    eth_debug = (eth_debug+1) & 0x3;
    DH2(eth_debug);
    DNL();

  } else if(in[1] == 'n') {
    ntp_sendpacket();

  }
}

void
dumppkt(void)
{
  uint8_t *a = uip_buf;

  DC('e');DC(' ');
  DU(uip_len,5);

  display_channel &= ~DISPLAY_TCP;
  uint8_t ole = log_enabled;
  log_enabled = 0;
  DC(' '); DC('d'); DC(' '); display_mac(a); a+= sizeof(struct uip_eth_addr);
  DC(' '); DC('s'); DC(' '); display_mac(a); a+= sizeof(struct uip_eth_addr);
  DC(' '); DC('t'); DH2(*a++); DH2(*a++);
  DNL();

  if(eth_debug > 2)
    dumpmem(a, uip_len - sizeof(struct uip_eth_hdr));
  display_channel |= DISPLAY_TCP;
  log_enabled = ole;
}

void
Ethernet_Task(void) {
     int i;

     ethernet_process();
     
     if(timer_expired(&periodic_timer)) {
	  timer_reset(&periodic_timer);
	  
	  for(i = 0; i < UIP_CONNS; i++) {
	       uip_periodic(i);
	       if(uip_len > 0) {
		    uip_arp_out();
		    network_send();
	       }
	  }
	  
	  for(i = 0; i < UIP_UDP_CONNS; i++) {
	       uip_udp_periodic(i);
	       if(uip_len > 0) {
		    uip_arp_out();
		    network_send();
	       }
	  }

	  interface_periodic();
	  
     }
     
     
     if(timer_expired(&arp_timer)) {
	  timer_reset(&arp_timer);
	  uip_arp_timer();
	  
     }
     
}

void                             // EEPROM Read IP
erip(void *ip, uint8_t *addr)
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

static void
ip_initialized(void)
{
  network_set_led(0x476);// LED A: Link Status  LED B: TX/RX
  tcplink_init();
  ntp_init();
#ifdef HAS_MDNS
  mdns_init();
#endif
}

void
dhcpc_configured(const struct dhcpc_state *s)
{
  if(s == 0) {
    set_eeprom_addr();
    return;
  }
  ewip(s->ipaddr,         EE_IP4_ADDR);    uip_sethostaddr(s->ipaddr);
  ewip(s->default_router, EE_IP4_GATEWAY); uip_setdraddr(s->default_router);
  ewip(s->netmask,        EE_IP4_NETMASK); uip_setnetmask(s->netmask);
  //resolv_conf(s->dnsaddr);
  uip_udp_remove(s->conn);
  ip_initialized();
}

static void
set_eeprom_addr()
{
  uip_ipaddr_t ipaddr;
  erip(ipaddr, EE_IP4_ADDR);    uip_sethostaddr(ipaddr);
  erip(ipaddr, EE_IP4_GATEWAY); uip_setdraddr(ipaddr);
  erip(ipaddr, EE_IP4_NETMASK); uip_setnetmask(ipaddr);
  ip_initialized();
}

void
tcp_appcall()
{
  if(uip_conn->lport == tcplink_port)
    tcplink_appcall();
}

void
udp_appcall()
{
  if(dhcp_state != PT_ENDED) {
    dhcp_state = handle_dhcp();

  } else if(uip_udp_conn &&
            uip_newdata() &&
            uip_udp_conn->lport == HTONS(NTP_PORT)) {
    ntp_digestpacket();

#ifdef HAS_MDNS
  } else if(uip_udp_conn &&
            uip_newdata() &&
            uip_udp_conn->lport == HTONS(MDNS_PORT)) {
    mdns_new_data();

#endif
  } 
}
