#ifndef __ETHERNET_H_
#define __ETHERNET_H_

#include "board.h"

void ethernet_reset(void);
void ethernet_init(void);
void Ethernet_Task(void);
extern uint8_t eth_initialized;
void erip(void *ip, uint8_t *addr);      // EEprom read IP

void udp_appcall(void);
void tcp_appcall(void);
void eth_func(char *);

// NOTE:
// typedef struct tcplink_state uip_tcp_appstate_t;
// typedef struct dhcpc_state uip_udp_appstate_t;

#define UIP_UDP_APPCALL udp_appcall
#define UIP_APPCALL     tcp_appcall

extern uint8_t eth_debug;

#endif
