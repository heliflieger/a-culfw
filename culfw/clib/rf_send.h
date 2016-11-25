#ifndef _RF_SEND_H
#define _RF_SEND_H


/* public prototypes */
void fs20send(char *in);
void rawsend(char *in);
void em_send(char *in);
void ks_send(char *in);
void ur_send(char *in);
void addParityAndSend(char *in, uint8_t startcs, uint8_t repeat);
void addParityAndSendData(uint8_t *hb, uint8_t hblen,
                        uint8_t startcs, uint8_t repeat);


extern uint16_t credit_10ms;
#define MAX_CREDIT 900       // max 9 seconds burst / 25% of the hourly budget

#endif
