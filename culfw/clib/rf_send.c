/* 
 * Copyright by R.Koenig
 * Inspired by code from Dirk Tostmann
 * License: GPL v2
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <util/parity.h>
#include <string.h>

#include "board.h"
#include "delay.h"
#include "rf_send.h"
#include "rf_receive.h"
#include "led.h"
#include "cc1100.h"
#include "display.h"
#include "fncollection.h"
#include "fht.h"

// For FS20 we time the complete message, for KS300 the rise-fall distance
// FS20  NULL: 400us high, 400us low
// FS20  ONE:  600us high, 600us low
// KS300 NULL  854us high, 366us low
// KS300 ONE:  366us high, 854us low

#define FS20_ZERO      400     //   400uS
#define FS20_ONE       600     //   600uS
#define FS20_PAUSE      10     // 10000mS

uint16_t credit_10ms;

#ifdef HAS_RAWSEND

static uint8_t zerohigh, zerolow, onehigh, onelow;
#define TMUL(x) (x<<4)
#define TDIV(x) (x>>4)

static void
send_bit(uint8_t bit)
{
  CC1100_OUT_PORT |= _BV(CC1100_OUT_PIN);         // High
  my_delay_us(bit ? TMUL(onehigh) : TMUL(zerohigh));

  CC1100_OUT_PORT &= ~_BV(CC1100_OUT_PIN);       // Low
  my_delay_us(bit ? TMUL(onelow) : TMUL(zerolow));
}

#else

static void
send_bit(uint8_t bit)
{
  CC1100_OUT_PORT |= _BV(CC1100_OUT_PIN);         // High
  my_delay_us(bit ? FS20_ONE : FS20_ZERO);

  CC1100_OUT_PORT &= ~_BV(CC1100_OUT_PIN);       // Low
  my_delay_us(bit ? FS20_ONE : FS20_ZERO);
}

#endif

static void sendraw(uint8_t *msg, uint8_t sync, uint8_t nbyte, uint8_t bitoff, 
                uint8_t repeat, uint8_t pause);

// msg is with parity/checksum already added
static void
sendraw(uint8_t *msg, uint8_t sync, uint8_t nbyte, uint8_t bitoff,
                uint8_t repeat, uint8_t pause)
{
  // 12*800+1200+nbyte*(8*1000)+(bits*1000)+800+10000 
  // message len is < (nbyte+2)*repeat in 10ms units.
  int8_t i, j, sum = (nbyte+2)*repeat;
  if (credit_10ms < sum) {
    DS_P(PSTR("LOVF\r\n"));
    return;
  }
  credit_10ms -= sum;

  LED_ON();
  if(!cc_on)
    set_ccon();
  ccTX();                       // Enable TX 
  do {
    for(i = 0; i < sync; i++)                   // sync
      send_bit(0);
    send_bit(1);
    
    for(j = 0; j < nbyte; j++) {                // whole bytes
      for(i = 7; i >= 0; i--)
        send_bit(msg[j] & _BV(i));
    }
    for(i = 7; i > bitoff; i--)                 // broken bytes
      send_bit(msg[j] & _BV(i));

    my_delay_ms(pause);                         // pause

  } while(--repeat > 0);

  if(tx_report) {                               // Enable RX
    ccRX();
  } else {
    ccStrobe(CC1100_SIDLE);
  }

  LED_OFF();
}

static int
abit(uint8_t b, uint8_t *obuf, uint8_t *obyp, uint8_t obi)
{
  uint8_t oby = * obyp;
  if(b)
    obuf[oby] |= _BV(obi);
  if(obi-- == 0) {
    oby++;
    if(oby < MAXMSG-1)
      *obyp = oby;
    obi = 7; obuf[oby] = 0;
  }
  return obi;
}

void
addParityAndSendData(uint8_t *hb, uint8_t hblen,
                uint8_t startcs, uint8_t repeat)
{
  uint8_t iby, obuf[MAXMSG], oby;
  int8_t ibi, obi;

  hb[hblen] = cksum1(startcs, hb, hblen);
  hblen++;

  // Copy the message and add parity-bits
  iby=oby=0;
  ibi=obi=7;
  obuf[oby] = 0;

  while(iby<hblen) {
    obi = abit(hb[iby] & _BV(ibi), obuf, &oby, obi);
    if(ibi-- == 0) {
      obi = abit(parity_even_bit(hb[iby]), obuf, &oby, obi);
      ibi = 7; iby++;
    }
  }
  if(obi-- == 0) {                   // Trailing 0 bit: no need for a check
    oby++; obi = 7;
  }

#ifdef HAS_RAWSEND
  zerohigh = zerolow = TDIV(FS20_ZERO);
  onehigh = onelow = TDIV(FS20_ONE);
#endif
  sendraw(obuf, 12, oby, obi, repeat, FS20_PAUSE);
}

void
addParityAndSend(char *in, uint8_t startcs, uint8_t repeat)
{
  uint8_t hb[MAXMSG], hblen;
  hblen = fromhex(in+1, hb, MAXMSG-1);
  addParityAndSendData(hb, hblen, startcs, repeat);
}


void
fs20send(char *in)
{
  addParityAndSend(in, 6, 3);
}

#ifdef HAS_RAWSEND
//G0843E540202040A78DE81D80
//G0843E540202040A78DE80F80


void
rawsend(char *in)
{
  uint8_t hb[16]; // 33/2: see ttydata.c
  uint8_t nby, nbi, pause, repeat, sync;

  fromhex(in+1, hb, sizeof(hb));
  sync = hb[0];
  nby = (hb[1] >> 4);
  nbi = (hb[1] & 0xf);
  pause = (hb[2] >> 4);
  repeat = (hb[2] & 0xf);
  zerohigh = hb[3];
  zerolow  = hb[4];
  onehigh  = hb[5];
  onelow   = hb[6];
  sendraw(hb+7, sync, nby, nbi, repeat, pause);
}
#endif
