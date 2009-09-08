#include <string.h>
#include "board.h"
#include "uip.h"
#include "display.h"
#include "ntp.h"
#include "timer.h"
#include "clock.h"
#include "fncollection.h"       // EE_IP4_GATEWAY
#include "ethernet.h"           // eth_debug

// Time of last sync.
time_t        ntp_sec = 3461476149U; // 2009-09-09 09:09:09 (GMT)
uint8_t       ntp_hsec;
 int8_t       ntp_gmtoff;
struct timer  ntp_timer;
static struct uip_udp_conn *ntp_conn = 0;


#define NTP_INTERVAL 6            // Max interval is 9 (512sec)

static void
fill_packet(ntp_packet_t *p)
{
  memset(p, 0, sizeof(p));
  p->li_vn_mode = 0x23;           // LI:00:No Warning, VN:04, Mode:03: Client
  p->poll = (1<<NTP_INTERVAL);
  p->refid.u32 = 0x4c4f434c;      // LOCL
  p->org_ts.u32[0] = ntp_sec;
  p->org_ts.u8[5]  = ntp_hsec*2;  // 255/125=2.04 -> up to 0.03sec diff
}

////////////////////////////////
void
ntp_sendpacket()
{
  if(ntp_conn == 0) {             // Initialize
    uip_ipaddr_t ipaddr;
    timer_set(&ntp_timer, (1<<NTP_INTERVAL)*CLOCK_SECOND); 
    erip(ipaddr, EE_IP4_NTPSERVER);
    if(ipaddr[0] == 0 && ipaddr[1] == 0)
      erip(ipaddr, EE_IP4_GATEWAY);
    ntp_conn = uip_udp_new(&ipaddr, HTONS(NTP_PORT));
    if(ntp_conn == NULL)
      return;
    uip_udp_bind(ntp_conn, HTONS(NTP_PORT));
  }

  if(eth_debug) {
    DC('s');
    DNL();
  }
  uip_udp_conn = ntp_conn;
  fill_packet((ntp_packet_t*)uip_appdata);
  uip_send(uip_appdata, sizeof(ntp_packet_t));
  timer_set(&ntp_timer, (1<<NTP_INTERVAL)*CLOCK_SECOND); 
}

void
ntpsec2tm(time_t sec, tm_t *t)
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
  uint8_t mday[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
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


////////////////////////////////
void
ntp_digestpacket()
{
  if(eth_debug)
    ntp_func(0);
  if(uip_len < sizeof(ntp_packet_t))
    return;
  ntp_packet_t *p = (ntp_packet_t*)uip_appdata;
  uint8_t *f = (uint8_t *)&ntp_sec;
  *f++  = p->tx_ts.u8[3];
  *f++  = p->tx_ts.u8[2];
  *f++  = p->tx_ts.u8[1];
  *f    = p->tx_ts.u8[0];
  ntp_hsec = p->tx_ts.u8[5] >> 2;
  if(eth_debug) {
    ntp_func(0);
    DNL();
  }
}

void
ntp_func(char *in)
{
  tm_t tm;
  ntpsec2tm(ntp_sec, &tm);
  display_udec(tm.tm_year,2,'0'); DC('-');
  display_udec(tm.tm_mon, 2,'0'); DC('-');
  display_udec(tm.tm_mday,2,'0'); DC(' ');
  display_udec(tm.tm_hour,2,'0'); DC(':');
  display_udec(tm.tm_min, 2,'0'); DC(':');
  display_udec(tm.tm_sec, 2,'0'); DC('.');
  display_udec(ntp_hsec,  3,'0');
  DNL();
}
