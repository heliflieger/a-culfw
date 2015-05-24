#include "uip.h"
#include <avr/pgmspace.h>
#include "uip_arp.h"            // uip_arp_out;
#include "drivers/interfaces/network.h"            // network_send
#include "delay.h"
#include <avr/eeprom.h>

#include <string.h>
#include <stdlib.h>

#include "board.h"
#include "version.h"
#include "ringbuffer.h"
#include "display.h"
#include "tcplink.h"
#include "fncollection.h"
#include "ttydata.h"

#define STATE_CLOSED 0
#define STATE_OPEN   1
#define STATE_CLOSE  2

static void senddata(u8_t rexmit);
uint16_t tcplink_port;
int con_counter;

/*---------------------------------------------------------------------------*/
void
tcplink_init(void)
{
  tcplink_port = HTONS(eeprom_read_word((uint16_t *)EE_IP4_TCPLINK_PORT));
  uip_listen(tcplink_port);
  con_counter = 0;
}

void
tcp_putchar(char data)
{
  struct tcplink_state *s;
  int c;
  
  for (c = 0; c < UIP_CONF_MAX_CONNECTIONS; ++c) {
    s = &(uip_conns[c].appstate);
    if(s->offset < sizeof(s->buffer))
      s->buffer[s->offset++] = data;
  }
}

void
tcplink_close(char *unused)
{
  struct tcplink_state *s = (struct tcplink_state *)&(uip_conn->appstate);

  s->state = STATE_CLOSE;
}


static void
tcpsend(void)
{
  struct tcplink_state *s = (struct tcplink_state *)&(uip_conn->appstate);

  while(TTY_Tx_Buffer.nbytes) {

    if(s->offset == sizeof(s->buffer)) {
      uip_process(UIP_TIMER);       // uip_send will be called
      uip_arp_out();
      network_send();
      my_delay_ms(2);
      Ethernet_Task();

    }
    s->buffer[s->offset++] = rb_get(&TTY_Tx_Buffer);

  }
}

/*---------------------------------------------------------------------------*/
static void
senddata(u8_t rexmit)
{
  struct tcplink_state *s = (struct tcplink_state *)&(uip_conn->appstate);

  if (rexmit != 0) {
    if(s->offsetlast == 0)
      return;
    
    // in case of rexmit, use bufferlast and offsetlast
    // do not touch current data in s->buffer
    memcpy(uip_appdata, s->bufferlast, s->offsetlast);
    uip_send(uip_appdata, s->offsetlast);

    #ifdef HAS_ETHERNET_KEEPALIVE
      // restart keepalive timer after uip_send() call
      timer_restart(&(s->tmr_keepalive));
    #endif
    
  } else {
  if(s->offset == 0)
    return;
    
  memcpy(uip_appdata, s->buffer, s->offset);
  uip_send(uip_appdata, s->offset);
    
    #ifdef HAS_ETHERNET_KEEPALIVE
      // restart keepalive timer after uip_send() call
      timer_restart(&(s->tmr_keepalive));
    #endif
    
    // copy current offset and data to bufferlast to be able rexmit identical data
    memcpy(s->bufferlast, s->buffer, s->offset);
    s->offsetlast = s->offset;
  s->offset = 0;
}
}

/*---------------------------------------------------------------------------*/
static void
closed(void)
{
  struct tcplink_state *s = (struct tcplink_state *)&(uip_conn->appstate);

  s->offset = 0;
  s->offsetlast = 0;
  s->state  = STATE_CLOSED;
}

/*---------------------------------------------------------------------------*/
static void
newdata(void)
{
  u16_t len;
  char *dataptr;

  len = uip_datalen();

  if(len<1)
   return;

  dataptr = (char *)uip_appdata;
  
  output_flush_func = tcpsend;

  while(len > 0) {
    rb_put(&TTY_Rx_Buffer, *dataptr);
    if(TTY_Rx_Buffer.nbytes == TTY_BUFSIZE)
      input_handle_func(DISPLAY_TCP);
    ++dataptr;
    --len;
  }
  input_handle_func(DISPLAY_TCP);
}

/*---------------------------------------------------------------------------*/
void
tcplink_appcall(void)
{
  //get momentary connection
  struct tcplink_state *s = (struct tcplink_state *)&(uip_conn->appstate);
	
  if(uip_connected()) {
    s->offset = 0;
    s->state = STATE_OPEN;
    if(con_counter++ == 0)
      display_channel |= DISPLAY_TCP;
    
    #ifdef HAS_ETHERNET_KEEPALIVE
      // initially start keepalive timer
      timer_set(&(s->tmr_keepalive), CLOCK_SECOND * ETHERNET_KEEPALIVE_TIME);
    #endif
  }

  if(s->state == STATE_CLOSE) {
    s->state = STATE_OPEN;
    uip_close();
    if(--con_counter == 0)
    	display_channel &= ~DISPLAY_TCP;
    return;
  }

  if(uip_closed() ||
    uip_aborted() ||
    uip_timedout()) {
      closed();
  }
  
// if(uip_acked()) {
//   acked();
// }
  
  if(uip_newdata()) {
    newdata();
  }
  
  if(uip_rexmit() ||
    uip_newdata() ||
    uip_acked() ||
    uip_connected() ||
    uip_poll()) {
      senddata(uip_rexmit());
  }
  
#ifdef HAS_ETHERNET_KEEPALIVE
  if ((s->state == STATE_OPEN) && (timer_expired(&(s->tmr_keepalive)))) {
    timer_reset(&(s->tmr_keepalive));
    
    // save current display channel
    uint8_t odc = display_channel;
    display_channel = DISPLAY_TCP;
    
    // issue a version information as simple keepalive packet
    version("V");
    
    // restore old display channel
    display_channel = odc;
  }
#endif
}
/*---------------------------------------------------------------------------*/
