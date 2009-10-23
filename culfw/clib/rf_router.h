#ifndef _RF_ROUTER_H
#define _RF_ROUTER_H


void rf_router_init(void);
void rf_router_func(char *);
void rf_router_reset(void);
void rf_router_send(void);
void rf_router_task(void);

extern uint8_t rf_router_status;
extern uint8_t rf_router_triggered;
extern uint8_t rf_router_id;

#define RF_ROUTER_DISABLED    0
#define RF_ROUTER_SYNC_RCVD   1
#define RF_ROUTER_DATA_WAIT   2
#define RF_ROUTER_ACK_WAIT    3

#define RF_WANTS_TRIGGER      2

#endif
