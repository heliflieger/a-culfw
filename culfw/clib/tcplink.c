#include "uip.h"
#include <avr/pgmspace.h>

#include <string.h>
#include <stdlib.h>

#include "board.h"
#include "version.h"
#include "ringbuffer.h"
#include "cdc.h"
#include "display.h"
#include "tcplink.h"
#include "fncollection.h"

#define STATE_CLOSED 0
#define STATE_OPEN   1
#define STATE_CLOSE  2

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
  	if(s->offset < 255)
	    s->buffer[s->offset++] = data;
  }   
}

void
tcplink_close(char *unused)
{
	struct tcplink_state *s = (struct tcplink_state *)&(uip_conn->appstate);

  s->state = STATE_CLOSE;
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

  if (len<1)
       return;

  dataptr = (char *)uip_appdata;
  
  while(len > 0) {
       rb_put(USB_Rx_Buffer, *dataptr);
       ++dataptr;
       --len;
  }

  usbinfunc();
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
    if (con_counter == 0) {
    	output_enabled |= OUTPUT_TCP;
    }
    ++con_counter;
  }

  if(s->state == STATE_CLOSE) {
    s->state = STATE_OPEN;
    uip_close();
    --con_counter;
    if (con_counter == 0) {
    	output_enabled &= ~OUTPUT_TCP;
    }
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
