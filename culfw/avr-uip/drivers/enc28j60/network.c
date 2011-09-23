#include "board.h"
#include "uip.h"
#include "uip_arp.h"
#include <avr/io.h>
#include <util/delay.h>
#include "enc28j60.h"

#define BUF ((struct uip_eth_hdr *)uip_buf)

unsigned int network_read(void){
	uint16_t len;
	len=enc28j60PacketReceive(UIP_BUFSIZE, (uint8_t *)uip_buf);
	return len;
}

void network_send(void){
	if(uip_len <= UIP_LLH_LEN + 40){
		enc28j60PacketSend(uip_len, (uint8_t *)uip_buf, 0, 0);
	}else{
		enc28j60PacketSend(54, (uint8_t *)uip_buf , uip_len - UIP_LLH_LEN - 40, (uint8_t*)uip_appdata);
	}
}

void network_init(void)
{
	//Initialise the device
	enc28j60Init();

	//Configure leds
	//enc28j60PhyWrite(PHLCON,0x476);
}

void network_get_MAC(u08* macaddr)
{
	// read MAC address registers
	// NOTE: MAC address in ENC28J60 is byte-backward
	*macaddr++ = enc28j60Read(MAADR5);
	*macaddr++ = enc28j60Read(MAADR4);
	*macaddr++ = enc28j60Read(MAADR3);
	*macaddr++ = enc28j60Read(MAADR2);
	*macaddr++ = enc28j60Read(MAADR1);
	*macaddr++ = enc28j60Read(MAADR0);
}

void network_set_MAC(u08* macaddr)
{
	// write MAC address
	// NOTE: MAC address in ENC28J60 is byte-backward
	enc28j60Write(MAADR5, *macaddr++);
	enc28j60Write(MAADR4, *macaddr++);
	enc28j60Write(MAADR3, *macaddr++);
	enc28j60Write(MAADR2, *macaddr++);
	enc28j60Write(MAADR1, *macaddr++);
	enc28j60Write(MAADR0, *macaddr++);
}

void network_set_led(uint16_t led) {
     enc28j60PhyWrite( PHLCON, led );
}



void ethernet_process(void) {
     
     uip_len = network_read();
     
     if(uip_len > 0) {
	  
	  if(uip_len > MAX_FRAMELEN) {
	       ethernet_init();
	       return;
	  }
      
//    if(eth_debug > 1)
//      dumppkt();

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
	  
     }

}

void interface_periodic(void) {
}
