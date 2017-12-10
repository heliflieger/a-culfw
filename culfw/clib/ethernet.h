#ifndef __ETHERNET_H_
#define __ETHERNET_H_

#include <stdint.h>

#include "board.h"
#include "ringbuffer.h"

void ethernet_reset(void);
void ethernet_init(void);
void Ethernet_Task(void);
extern uint8_t eth_initialized;
void erip(void *ip, uint8_t *addr);      // EEprom read IP

void udp_appcall(void);
void tcp_appcall(void);
void eth_func(char *);

#ifdef HAS_WIZNET
extern rb_t NET_Tx_Buffer;
void NET_Receive_next (uint8_t socket_num);
void Net_Write(uint8_t *data, uint16_t size,  uint8_t socket);
uint8_t check_Net_MAC();

#define NET1  0
#define NET2  1
#endif

// NOTE:
// typedef struct tcplink_state uip_tcp_appstate_t;
// typedef struct dhcpc_state uip_udp_appstate_t;

#define UIP_UDP_APPCALL udp_appcall
#define UIP_APPCALL     tcp_appcall

extern uint8_t eth_debug;

#endif
