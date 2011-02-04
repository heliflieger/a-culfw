#ifndef _RF_SEND_H
#define _RF_SEND_H


/* public prototypes */
void fs20send(char *in);
void rawsend(char *in);
void it_func(char *in);
void em_send(char *in);
void addParityAndSend(char *in, uint8_t startcs, uint8_t repeat);
void addParityAndSendData(uint8_t *hb, uint8_t hblen,
                        uint8_t startcs, uint8_t repeat);


extern uint16_t credit_10ms;
#ifdef HAS_IRRX
extern uint8_t rf_send_active;
#endif
#define MAX_CREDIT 900       // max 9 seconds burst / 25% of the hourly budget

#endif
