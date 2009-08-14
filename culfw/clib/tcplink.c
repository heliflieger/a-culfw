/*
 * Copyright (c) 2003, Adam Dunkels.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * This file is part of the uIP TCP/IP stack
 *
 * $Id: tcplink.c,v 1.1 2009-08-14 22:19:29 tostmann Exp $
 *
 */

#include "uip.h"
#include "tcplink.h"
#include <avr/pgmspace.h>

#include <string.h>
#include <stdlib.h>

#include "board.h"
#include "version.h"
#include "ringbuffer.h"
#include "cdc.h"

static struct tcplink_state s;

#define STATE_CLOSED 0
#define STATE_OPEN   1
#define STATE_CLOSE  2

/*---------------------------------------------------------------------------*/
void tcplink_init(void) {
     uip_listen(HTONS(2323));
     s.buffer = NULL;
     s.state  = STATE_CLOSED;
}

void tcp_putchar(char data) {
     if (!s.buffer)
	  return;
     
     strncat( s.buffer, &data, 1 );
}

void tcplink_close(char *unused) {
     s.state = STATE_CLOSE;
}


/*---------------------------------------------------------------------------*/
static void senddata(void) {
     static char *bufptr;
     static int buflen;

     buflen = strlen(s.buffer);

     if (buflen<1)
	  return;
     
     bufptr = uip_appdata;

     memcpy(bufptr, s.buffer, buflen);
     *s.buffer = 0;

     uip_send(uip_appdata, buflen);
}
/*---------------------------------------------------------------------------*/
static void closed(void) {
     if (s.buffer)
	  free( s.buffer );
     s.buffer = NULL;
     s.state  = STATE_CLOSED;
}

/*---------------------------------------------------------------------------*/
static void newdata(void) {

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
void tcplink_appcall(void) {

     if(uip_connected()) {
	  s.buffer = malloc( 50 );
	  *s.buffer = 0;
	  
	  sprintf_P( s.buffer, PSTR("Welcome to " BOARD_ID_STR " running culfw V " VERSION "\r\n" ) );
	  
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
     
//  if(uip_acked()) {
//    acked();
//  }
     
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
