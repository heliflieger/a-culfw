#include "uip.h"
#include <avr/pgmspace.h>
#include "uip_arp.h"            // uip_arp_out;
#include "drivers/interfaces/network.h"            // network_send
#include "delay.h"

#include <string.h>
#include <stdlib.h>

#include "board.h"
#include "version.h"
#include "ringbuffer.h"
#include "cdc.h"
#include "display.h"
#include "tcplink.h"
#include "fncollection.h"
#include "ttydata.h"

#define STATE_CLOSED 0
#define STATE_OPEN   1
#define STATE_CLOSE  2

static void senddata(void);
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
senddata(void)
{
  struct tcplink_state *s = (struct tcplink_state *)&(uip_conn->appstate);

  if(s->offset == 0)
    return;
  memcpy(uip_appdata, s->buffer, s->offset);
  uip_send(uip_appdata, s->offset);
  s->offset = 0;
}

/*---------------------------------------------------------------------------*/
static void
closed(void)
{
  struct tcplink_state *s = (struct tcplink_state *)&(uip_conn->appstate);

  s->offset = 0;
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
  
  uint8_t odc = display_channel;
  output_flush_func = tcpsend;
  display_channel = DISPLAY_TCP;

  while(len > 0) {
    rb_put(&TTY_Rx_Buffer, *dataptr);
    if(TTY_Rx_Buffer.nbytes == TTY_BUFSIZE)
      input_handle_func();
    ++dataptr;
    --len;
  }
  input_handle_func();
  display_channel = odc;
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
      senddata();
  }
}
/*---------------------------------------------------------------------------*/
