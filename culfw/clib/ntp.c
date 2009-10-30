#include <string.h>
#include "board.h"
#include "uip.h"
#include "uip_arp.h"            // uip_arp_out;
#include "drivers/interfaces/network.h"            // network_send
#include "display.h"
#include "ntp.h"
#include "clock.h"
#include "fncollection.h"       // EE_IP4_GATEWAY
#include "ethernet.h"           // eth_debug

// Time of last sync.
time_t        ntp_sec = 3461476149U; // 2009-09-09 09:09:09 (GMT)
uint8_t       ntp_hsec;
 int8_t       ntp_gmtoff;
struct uip_udp_conn *ntp_conn = 0;

////////////////////////////////
// Network part
static void
fill_packet(ntp_packet_t *p)
{
  memset(p, 0, sizeof(*p));
  p->li_vn_mode = 0x23;           // LI:00:No Warning, VN:04, Mode:03: Client
}

void
ntp_init(void)
{
  uip_ipaddr_t ipaddr;

  ntp_gmtoff = erb(EE_IP4_NTPOFFSET);
  erip(ipaddr, EE_IP4_NTPSERVER);
  if(ipaddr[0] == 0 && ipaddr[1] == 0)
    erip(ipaddr, EE_IP4_GATEWAY);
  ntp_conn = uip_udp_new(&ipaddr, HTONS(NTP_PORT));
  if(ntp_conn == NULL)
    return;
  uip_udp_bind(ntp_conn, HTONS(NTP_PORT));
  //ntp_sendpacket();
}

////////////////////////////////
void
ntp_sendpacket()
{
  if(eth_debug) {
    DC('n'); DC('s'); ntp_func(0);
  }

  if(ntp_conn == 0)
    return;

  uip_udp_conn = ntp_conn;
  fill_packet((ntp_packet_t*)uip_appdata);
  uip_send(uip_appdata, sizeof(ntp_packet_t));

  uip_process(UIP_UDP_SEND_CONN);
  uip_arp_out();
  network_send();
}

////////////////////////////////
void
ntp_digestpacket()
{
  if(uip_len < sizeof(ntp_packet_t))
    return;
  ntp_packet_t *p = (ntp_packet_t*)uip_appdata;
  uint8_t *f = (uint8_t *)&ntp_sec;
  f[0] = p->tx_ts.u8[3];
  f[1] = p->tx_ts.u8[2];
  f[2] = p->tx_ts.u8[1];
  f[3] = p->tx_ts.u8[0];
  ntp_hsec = (uint16_t)(p->tx_ts.u8[4]*125)/256;
  if(eth_debug) {
    DC('n'); DC('r'); ntp_func(0);
  }
}



/////////////////////////////////////////
// Helper functions / conversion to/from readable format

uint8_t mday[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

uint8_t
dec2bcd(uint8_t in)
{
  return ((in/10)<<4) | (in%10);
}

uint8_t
bcd2dec(uint8_t in)
{
  return ((in>>4)*10)+(in&0xf);
}


void
ntp_sec2tm(time_t sec, tm_t *t)
{
  uint8_t m, y;
  sec += (ntp_gmtoff*3600);
  uint16_t day = (uint16_t)(sec / 86400) - 39812;

  /////////// Year
  for(y=9;;y++) {
    uint16_t diy = (y&3 ? 365 : 366);
    if(day < diy)
      break;
    day -= diy;
  }

  /////////// Month/Day
  for(m = 0;;m++) {
    uint8_t md = (m==1 && (y&3) == 0) ? 29 : mday[m];
    if(day < md)
      break;
    day -= md;
  }

  t->tm_year = y;
  t->tm_mon  = m+1;
  t->tm_mday = day+1;
  t->tm_hour = (sec / 3600) % 24;
  t->tm_min  = (sec / 60) % 60;
  t->tm_sec  = sec % 60;
}

time_t
ntp_tm2sec(tm_t *t)
{
  uint8_t m, y;
  uint16_t day = 39812;

  for(y=9; y < t->tm_year ;y++)
    day += (y&3 ? 365 : 366);

  for(m = 0; m < t->tm_mon-1; m++)
    day += (m==1 && (y&3) == 0) ? 29 : mday[m];

  day += (t->tm_mday-1);

  return (((time_t)day*24+
                  (t->tm_hour-ntp_gmtoff))*60+
                   t->tm_min)*60+
                   t->tm_sec;
}


void
ntp_func(char *in)
{
  uint8_t hb[6], t;

  if(in == 0 || in[1] == 0) {
    t = 1, hb[0] = 7;
  } else  {
    t = fromhex(in+1, hb, 6);
  }

  if(t == 1) {
    t = hb[0];
    ntp_get(hb);
    if(t&1) {
      DH2(hb[0]); DC('-');
      DH2(hb[1]); DC('-');
      DH2(hb[2]);
    }
    if((t&3) == 3)
      DC(' ');

    if(t&2) {
      DH2(hb[3]); DC(':');
      DH2(hb[4]); DC(':');
      DH2(hb[5]);
    }

    if(t&4) {
      DC('.'); display_udec((uint16_t)(ntp_hsec*100)/125, 2, '0');
    }
    DNL();

  } else if(t == 6) {
    tm_t t;
    t.tm_year = bcd2dec(hb[0]);
    t.tm_mon  = bcd2dec(hb[1]);
    t.tm_mday = bcd2dec(hb[2]);
    t.tm_hour = bcd2dec(hb[3]);
    t.tm_min  = bcd2dec(hb[4]);
    t.tm_sec  = bcd2dec(hb[5]);
    ntp_sec = ntp_tm2sec(&t);

  }
}

void
ntp_get(uint8_t data[6])                // for the log function
{
  tm_t t;
  ntp_sec2tm(ntp_sec, &t);
  data[0] = dec2bcd(t.tm_year);
  data[1] = dec2bcd(t.tm_mon);
  data[2] = dec2bcd(t.tm_mday);
  data[3] = dec2bcd(t.tm_hour);
  data[4] = dec2bcd(t.tm_min);
  data[5] = dec2bcd(t.tm_sec);
}
