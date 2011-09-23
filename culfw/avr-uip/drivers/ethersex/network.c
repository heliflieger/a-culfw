#include "board.h"
#include "uip.h"
#include "uip_arp.h"
#include <avr/io.h>
#include <util/delay.h>
#include "enc28j60.h"
#include "bit-macros.h"

#define ENC28J60_REV4_WORKAROUND

//struct timer periodic_timer, arp_timer;

void network_init(void) {
     //Initialise the device
     init_enc28j60();
}

void network_get_MAC(uint8_t* macaddr) {
     // read MAC address registers
     // NOTE: MAC address in ENC28J60 is byte-backward
     *macaddr++ = read_control_register(REG_MAADR5);
     *macaddr++ = read_control_register(REG_MAADR4);
     *macaddr++ = read_control_register(REG_MAADR3);
     *macaddr++ = read_control_register(REG_MAADR2);
     *macaddr++ = read_control_register(REG_MAADR1);
     *macaddr++ = read_control_register(REG_MAADR0);
}

void network_set_MAC(uint8_t* macaddr) {
     // write MAC address
     // NOTE: MAC address in ENC28J60 is byte-backward
     write_control_register(REG_MAADR5, *macaddr++);
     write_control_register(REG_MAADR4, *macaddr++);
     write_control_register(REG_MAADR3, *macaddr++);
     write_control_register(REG_MAADR2, *macaddr++);
     write_control_register(REG_MAADR1, *macaddr++);
     write_control_register(REG_MAADR0, *macaddr++);
}

void network_set_led(uint16_t led) {
     write_phy( PHY_PHLCON, led );
}


void network_send(void) {

    /* wait for any transmits to end, with timeout */
    uint8_t timeout = 100;
    while (read_control_register(REG_ECON1) & _BV(ECON1_TXRTS) && timeout-- > 0);

    if (timeout == 0) {
//        debug_printf("net: timeout waiting for TXRTS, aborting transmit!\n");
        return;
    }

    uint16_t start_pointer = TXBUFFER_START;

    /* set send control registers */
    write_control_register(REG_ETXSTL, LO8(start_pointer));
    write_control_register(REG_ETXSTH, HI8(start_pointer));

    write_control_register(REG_ETXNDL, LO8(start_pointer + uip_len));
    write_control_register(REG_ETXNDH, HI8(start_pointer + uip_len));

    /* set pointer to beginning of tx buffer */
    set_write_buffer_pointer(start_pointer);

    /* write override byte */
    write_buffer_memory(0);

    /* write data */
    for (uint16_t i = 0; i < uip_len; i++)
        write_buffer_memory(uip_buf[i]);

#   ifdef ENC28J60_REV4_WORKAROUND
    /* reset transmit hardware, see errata #12 */
    bit_field_set(REG_ECON1, _BV(ECON1_TXRST));
    bit_field_clear(REG_ECON1, _BV(ECON1_TXRST));
#   endif

    /* transmit packet */
    bit_field_set(REG_ECON1, _BV(ECON1_TXRTS));

}

unsigned int network_read(void) {

    /* if there is a packet to process */
    if (read_control_register(REG_EPKTCNT) == 0)
        return 0;

#   ifdef DEBUG_NET
    debug_printf("net: packet received\n");
#   endif

    /* read next packet pointer */
    set_read_buffer_pointer(enc28j60_next_packet_pointer);
    enc28j60_next_packet_pointer = read_buffer_memory() | (read_buffer_memory() << 8);

    /* read receive status vector */
    struct receive_packet_vector_t rpv;
    uint8_t *p = (uint8_t *)&rpv;

    for (uint8_t i = 0; i < sizeof(struct receive_packet_vector_t); i++)
        *p++ = read_buffer_memory();

    /* decrement rpv received_packet_size by 4, because the 4 byte CRC checksum is counted */
    rpv.received_packet_size -= 4;

    /* check size */
    if (rpv.received_packet_size > NET_MAX_FRAME_LENGTH
            || rpv.received_packet_size < 14
            || rpv.received_packet_size > UIP_BUFSIZE) {
#       ifdef DEBUG
        debug_printf("net: packet too large or too small for an "
		     "ethernet header: %d\n", rpv.received_packet_size);
#       endif
	init_enc28j60();
        return 0;
    }

    /* read packet */
    p = uip_buf;
    for (uint16_t i = 0; i < rpv.received_packet_size; i++)
        *p++ = read_buffer_memory();

    uip_len = rpv.received_packet_size;

    /* Set the enc stack active */
//    uip_stack_set_active(STACK_ENC);

    /* process packet */
    struct uip_eth_hdr *packet = (struct uip_eth_hdr *)&uip_buf;

    switch (HTONS(packet->type)) {

        /* process arp packet */
        case UIP_ETHTYPE_ARP:
            uip_arp_arpin();

            /* if there is a packet to send, send it now */
            if (uip_len > 0)
		 network_send();

            break;

        /* process ip packet */
        case UIP_ETHTYPE_IP:

	     uip_arp_ipin();

	     uip_arp_ipin();
	     uip_input();

	     if(uip_len > 0) {
		  uip_arp_out();
		  network_send();
	     }

            break;
    }

    /* advance receive read pointer, ensuring that an odd value is programmed
     * (next_receive_packet_pointer is always even), see errata #13 */
    if ( (enc28j60_next_packet_pointer - 1) < RXBUFFER_START
            || (enc28j60_next_packet_pointer - 1) > RXBUFFER_END) {

        write_control_register(REG_ERXRDPTL, LO8(RXBUFFER_END));
        write_control_register(REG_ERXRDPTH, HI8(RXBUFFER_END));

    } else {

        write_control_register(REG_ERXRDPTL, LO8(enc28j60_next_packet_pointer - 1));
        write_control_register(REG_ERXRDPTH, HI8(enc28j60_next_packet_pointer - 1));

    }

    /* decrement packet counter */
    bit_field_set(REG_ECON2, _BV(PKTDEC));

    return 1;
}

void ethernet_process(void) {

    /* also check packet counter, see errata #6 */
#   ifdef ENC28J60_REV4_WORKAROUND
    uint8_t pktcnt = read_control_register(REG_EPKTCNT);
#   endif

    /* if no interrupt occured and no packets are in the receive
     * buffer, return */
#   ifdef ENC28J60_REV4_WORKAROUND
    if ( pktcnt == 0 )
	 return;
#   endif

    /* read interrupt register */
    uint8_t EIR = read_control_register(REG_EIR);

    /* clear global interrupt flag */
    bit_field_clear(REG_EIE, _BV(INTIE));

#ifdef DEBUG_INTERRUPT
    /* check if some interrupts occured */
    if (EIR != 0) {

        debug_printf("net: controller interrupt, EIR = 0x%02x\n", EIR);
        if (EIR & _BV(LINKIF))
            debug_printf("\t* Link\n");
        if (EIR & _BV(TXIF))
            debug_printf("\t* Tx\n");
        if (EIR & _BV(PKTIF))
            debug_printf("\t* Pkt\n");
        if (EIR & _BV(RXERIF))
            debug_printf("\t* rx error\n");
        if (EIR & _BV(TXERIF))
            debug_printf("\t* tx error\n");
    }
#endif

    /* check each interrupt flag the interrupt is activated for, and clear it
     * if neccessary */

#if 0

    /* link change flag */
    if (EIR & _BV(LINKIF)) {

        /* clear interrupt flag */
        read_phy(PHY_PHIR);

        /* read new link state */
        uint8_t link_state = (read_phy(PHY_PHSTAT2) & _BV(LSTAT)) > 0;

			if (link_state) {
				debug_printf("net: got link!\n");
				#ifdef STATUSLED_NETLINK_SUPPORT
				PIN_SET(STATUSLED_NETLINK);
				#endif
			} else {
				debug_printf("net: no link!\n");
				#ifdef STATUSLED_NETLINK_SUPPORT
				PIN_CLEAR(STATUSLED_NETLINK);
				#endif
			}
    }

#endif

    /* packet transmit flag */
    if (EIR & _BV(TXIF)) {

#ifdef DEBUG
        uint8_t ESTAT = read_control_register(REG_ESTAT);

        if (ESTAT & _BV(TXABRT))
            debug_printf("net: packet transmit failed\n");
#endif
        /* clear flags */
        bit_field_clear(REG_EIR, _BV(TXIF));
        bit_field_clear(REG_ESTAT, _BV(TXABRT) | _BV(LATECOL) );
    }

    /* packet receive flag */
    if ( (EIR & _BV(PKTIF)) || pktcnt ) {

//      if (uip_buf_lock ())
//	return;			/* already locked */

      network_read();
//      uip_buf_unlock ();
    }

    /* receive error */
    if (EIR & _BV(RXERIF)) {
//        debug_printf("net: receive error!\n");

        bit_field_clear(REG_EIR, _BV(RXERIF));

/* not needed anymore ?? */

#ifdef ENC28J60_REV4_WORKAROUND
//        init_enc28j60();
#endif

    }

    /* transmit error */
    if (EIR & _BV(TXERIF)) {
#ifdef DEBUG
        debug_printf("net: transmit error!\n");
#endif

        bit_field_clear(REG_EIR, _BV(TXERIF));
    }

    /* set global interrupt flag */
    bit_field_set(REG_EIE, _BV(INTIE));
}


void interface_periodic(void) {
     uint8_t mask = _BV(PADCFG0) | _BV(TXCRCEN) | _BV(FRMLNEN);
     
     if (   ((read_control_register(REG_MACON3) & mask) != mask  ) || ( read_control_register(REG_MACON1) != 0x0D  ) ) {
	  init_enc28j60();
     }
}


#if 0

** OLD **

void ethernet_process(void) {
  int i;
  
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
    
  } else if(timer_expired(&periodic_timer)) {
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
       
    if(timer_expired(&arp_timer)) {
      timer_reset(&arp_timer);
      uip_arp_timer();
         
    }

  }

}

#endif
