/*
 * Copyright (c) 2003, Adam Dunkels.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
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
 * $Id: tcplink.h,v 1.5 2009-11-21 09:04:21 rudolfkoenig Exp $
 *
 */
#ifndef __TCPLINK_H__
#define __TCPLINK_H__

#include "uipopt.h"
#include "board.h"

void tcplink_appcall(void);
void tcp_putchar(char data);
void tcplink_close(char *unused);
void tcplink_init(void);

struct tcplink_state {
  u8_t state;
#ifdef HAS_ETHERNET_KEEPALIVE
  struct timer tmr_keepalive;
#endif
#ifdef HAS_VZ
  u16_t offset;
  char buffer[VZ_MSG_SIZE];
  u16_t offsetlast;
  char bufferlast[VZ_MSG_SIZE];
#else
  u8_t offset;
  char buffer[250];
  u8_t offsetlast;
  char bufferlast[250];
#endif
};

typedef struct tcplink_state uip_tcp_appstate_t;

extern uint16_t tcplink_port;

#endif /* __TCPLINK_H__ */
