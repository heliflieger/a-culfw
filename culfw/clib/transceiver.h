#ifndef _TRANSCEIVER_H
#define _TRANSCEIVER_H

#include <MyUSB/Scheduler/Scheduler.h> // Simple scheduler for task management

#define TYPE_EM      'E'
#define TYPE_HRM     'H'        // Hoermann
#define TYPE_FHT     'T'
#define TYPE_FS20    'F'
#define TYPE_KS300   'K'

#define REP_KNOWN    _BV(0)
#define REP_REPEATED _BV(1)
#define REP_BITS     _BV(2)
#define REP_MONITOR  _BV(3)
#define REP_BINTIME  _BV(4)
#define REP_RSSI     _BV(5)

/* public prototypes */
void set_txreport(char *in);
void fs20send(char *in);
void rawsend(char *in);
void addParityAndSend(char *in, uint8_t startcs, uint8_t repeat);
void addParityAndSendData(uint8_t *hb, uint8_t hblen,
                        uint8_t startcs, uint8_t repeat);

void set_txoff(void);
void set_txon(void);
void set_txrestore(void);

extern uint8_t tx_report;

extern uint16_t credit_10ms;
#define MAX_CREDIT 500          // five second burst

TASK(RfAnalyze_Task);

#endif
