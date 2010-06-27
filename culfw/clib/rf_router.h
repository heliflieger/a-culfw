#ifndef _RF_ROUTER_H
#define _RF_ROUTER_H

#include "ringbuffer.h"

void rf_router_init(void);
void rf_router_func(char *);
void rf_router_task(void);
void rf_router_flush(void);

extern uint8_t rf_router_status;
extern uint8_t rf_router_target;        // id of the router
extern uint8_t rf_router_myid;          // Own id
extern uint8_t rf_router_sendtime;      // relative ticks
extern uint8_t rf_nr_send_checks;

#ifdef RFR_SHADOW
#define RFR_Buffer TTY_Tx_Buffer
#endif
extern rb_t RFR_Buffer;

#define RF_ROUTER_PROTO_ID    'u'      // 117 / 0x75

#define RF_ROUTER_INACTIVE    0
#define RF_ROUTER_SYNC_RCVD   1
#define RF_ROUTER_DATA_WAIT   2
#define RF_ROUTER_GOT_DATA    3
#define RF_ROUTER_SENDING     4

#endif
