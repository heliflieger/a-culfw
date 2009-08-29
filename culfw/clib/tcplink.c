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

#define s uip_conns[0].appstate

/*---------------------------------------------------------------------------*/
void
tcplink_init(void)
{
  uip_listen(HTONS(eeprom_read_word((uint16_t *)EE_IP4_TCPLINK_PORT)));
}

void
tcp_putchar(char data)
{
  if(s.offset < 255)
    s.buffer[s.offset++] = data;
}

void
tcplink_close(char *unused)
{
  s.state = STATE_CLOSE;
}



/*---------------------------------------------------------------------------*/
static void
senddata(void)
{
  if(s.offset == 0)
    return;
  memcpy(uip_appdata, s.buffer, s.offset);
  uip_send(uip_appdata, s.offset);
  s.offset = 0;
}

/*---------------------------------------------------------------------------*/
static void
closed(void)
{
  s.offset = 0;
  s.state  = STATE_CLOSED;
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
  if(uip_connected()) {
    s.offset = 0;
    s.state = STATE_OPEN;
  }

  if(s.state == STATE_CLOSE) {
    s.state = STATE_OPEN;
    uip_close();
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
