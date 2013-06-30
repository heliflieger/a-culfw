#ifndef _RF_RECEIVE_H
#define _RF_RECEIVE_H


#define TYPE_EM      'E'
#define TYPE_HMS     'H'
#define TYPE_FHT     'T'
#define TYPE_FS20    'F'
#define TYPE_KS300   'K'
#define TYPE_HRM     'R'        // Hoermann
#define TYPE_ESA     'S'
#define TYPE_TX3     't'

#define REP_KNOWN    _BV(0)
#define REP_REPEATED _BV(1)
#define REP_BITS     _BV(2)
#define REP_MONITOR  _BV(3)
#define REP_BINTIME  _BV(4)
#define REP_RSSI     _BV(5)
#define REP_FHTPROTO _BV(6)
#define REP_LCDMON   _BV(7)

#ifndef REPTIME
#define REPTIME      38
#endif

/* public prototypes */
#ifdef HAS_ESA
#define MAXMSG 20               // ESA messages
#else
#define MAXMSG 12               // EMEM messages
#endif

void set_txreport(char *in);
void set_txrestore(void);
void tx_init(void);
uint8_t rf_isreceiving(void);
uint8_t cksum1(uint8_t s, uint8_t *buf, uint8_t len);
uint8_t cksum2(uint8_t *buf, uint8_t len);

extern uint8_t tx_report;

void RfAnalyze_Task(void);

#endif
