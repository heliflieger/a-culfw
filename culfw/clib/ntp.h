#ifndef __NTP_H
#define __NTP_H

typedef uint32_t time_t;

typedef union {
  uint32_t u32;
  uint16_t u16[2];
  uint8_t  u8[4];
} ts_32_t;

typedef union {
  uint32_t u32[2];
  uint16_t u16[4];
  uint8_t  u8[8];
} ts_64_t;

typedef struct {
  uint8_t    li_vn_mode;          /* leap indicator, version and mode */
  uint8_t    stratum;
  uint8_t    poll;
  int8_t     precision;
  ts_32_t    delay;
  ts_32_t    dispersion;
  ts_32_t    refid;
  ts_64_t    ref_ts;
  ts_64_t    org_ts;
  ts_64_t    rcv_ts;
  ts_64_t    tx_ts;
} ntp_packet_t;

typedef struct tm {
  uint8_t    tm_year;
  uint8_t    tm_mon;
  uint8_t    tm_mday;
  uint8_t    tm_hour;
  uint8_t    tm_min;
  uint8_t    tm_sec;
} tm_t;

void ntp_init(void);
void ntp_sendpacket(void);
void ntp_digestpacket(void);
void ntp_func(char *in);
void ntp_get(uint8_t now[6]);
void ntp_sec2tm(time_t sec, tm_t *t);
time_t ntp_tm2sec(tm_t *t);

extern time_t   ntp_sec;
extern uint8_t  ntp_hsec;
extern  int8_t  ntp_gmtoff;
extern struct uip_udp_conn *ntp_conn;

#define NTP_PORT 123
#define NTP_INTERVAL 8            // Max interval is 9 (512sec)
#define NTP_INTERVAL_MASK 0xff    // 264sec = 4.5min

#endif
