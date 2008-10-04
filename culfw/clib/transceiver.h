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
void tx_init(void);
void set_txreport(char *);
void fs20send(char *);
void fhtsend(char *);

extern uint8_t tx_report;

extern int16_t credit_10ms;
#define MAX_CREDIT 500          // five second burst

TASK(RfAnalyze_Task);

// One bucket to collect the "raw" bits
typedef struct {
     uint8_t state, sync, byteidx, bitidx;
     uint8_t data[12];          // contains parity and checksum, but no sync
     uint16_t zero;             // measured zero duration
     uint16_t avg;              // (zero+one/2), used to decide 0 or 1
} bucket_t;

#endif
