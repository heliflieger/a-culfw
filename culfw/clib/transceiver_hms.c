/* vim:fdm=marker ts=2 ai
 *
 * Copyright by R.Koenig, D.Tostmann, M.Mueller
 * License: GPL v2
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <avr/eeprom.h>
#include <util/parity.h>
#include <string.h>

#include "board.h"
#include "delay.h"
#include "transceiver.h"
#include "led.h"
#include "cc1100.h"
#include "display.h"
#include "clock.h"
#include "fncollection.h"
#include "fht.h"

// For FS20 we time the complete message, for KS300 the rise-fall distance
// FS20  NULL: 400us high, 400us low
// FS20  ONE:  600us high, 600us low
// KS300 NULL  854us high, 366us low
// KS300 ONE:  366us high, 854us low

#define FS20_ZERO      400    //   400uS
#define FS20_ONE       600    //   600uS
#define FS20_PAUSE      10    // 10000mS
#define MAXMSG		17    // For CUR request we need, inclusive parity (Netto 15)

uint16_t credit_10ms;
uint8_t tx_report;              // global verbose / output-filter
static uint8_t	oby, obuf[MAXMSG];	// parity-stripped output
static uint16_t wait_high_zero, wait_low_zero, wait_high_one, wait_low_one;
static uint8_t cc_on;

static void send_bit(uint8_t bit);
static void sendraw(uint8_t msg[], uint8_t nbyte, uint8_t bitoff,
                    uint8_t repeat, uint8_t pause);

static uint8_t cksum1(uint8_t s, uint8_t *buf, uint8_t len);

/* ---- new tranceiver stuff from Dirk Tostmann ---- */
#define BITWIN		200
#define EDGETIMEOUT 2500
#define AD_Q_MX		1

typedef struct {
	uint16_t high, low;
} bit_t;

typedef struct {
	uint8_t	bits, sync, state;
	uint8_t	data[MAXMSG];
	bit_t	bit[2];
	bit_t	sum[2];
} airdata_t;

typedef struct {
	uint8_t dec[MAXMSG];
	uint8_t bytes, type;
} airmsg_t;

volatile airdata_t	ad;
volatile uint16_t	bl, bh;
airdata_t			adq[AD_Q_MX];
airmsg_t			last_msg;
uint32_t			last_msg_t;
uint8_t				adc = 0;

void
AD_SET_BIT( volatile airdata_t *d, uint8_t b )
{
  d->data[b>>3] |= _BV(b&7);
}

#if 1

void
AD_PUSH_1( volatile airdata_t *d)
{
  AD_SET_BIT( d, d->bits );
  if ( d->bits < ( MAXMSG << 3 ))
  d->bits++;
  else {
  	DS_P(PSTR("EOVF\r\n"));
		memset((void *)d , 0, sizeof(airdata_t));
		bl = 0;
		bh = 0;
  }
}

void
AD_PUSH_0(volatile airdata_t *d)
{
  if ( d->bits < ( MAXMSG << 3 ))
  d->bits++;
  else {
  	DS_P(PSTR("EOVF\r\n"));
		memset((void *)d , 0, sizeof(airdata_t));
		bl = 0;
		bh = 0;
  }
}

#else

void
AD_PUSH( volatile airdata_t *d, uint8_t b)
{
  if(b)
    AD_SET_BIT( d, d->bits );
  d->bits++;
}

#define AD_PUSH_0(a) AD_PUSH(a, 0)
#define AD_PUSH_1(a) AD_PUSH(a, 1)

#endif

uint8_t
AD_BIT_IS_SET( airdata_t *d, uint8_t b )
{
  return bit_is_set(d->data[b>>3], b&7);
}

uint8_t
AD_SHL( airdata_t *d )
{
  uint8_t val = 0;

  if (d->bits) {
    val = d->data[0] & 1;
    d->bits--;
    d->data[0] = d->data[0] >> 1;
    // move the bits
    for (uint8_t b=0;b<(d->bits>>3);b++) {
      if (bit_is_set(d->data[b+1],0))
        d->data[b] = d->data[b] | 0x80;
      d->data[b+1] = d->data[b+1] >> 1;
    }
  }
  return val;
}

uint16_t
AD_POP( airdata_t *d, uint8_t b, uint8_t msb )
{
  uint16_t val = 0;

  for (uint8_t i=0;i<b;i++) {
    if (AD_SHL( d ))
      val = val | _BV( msb ? b-i-1 : i );
  }
  return val;
}

uint8_t
looks_like( bit_t a, bit_t b )
{
  int16_t dbl = a.low-b.low;
  int16_t dbh = a.high-b.high;
  int16_t dp  = (a.low+a.high) - (b.low+b.high);
  if ( dp<BITWIN && dp>-BITWIN && dbl<BITWIN && dbl>-BITWIN &&
       dbh<BITWIN && dbh>-BITWIN )
    return 1;
  return 0;
}

uint8_t
looks_like_s( bit_t a, uint16_t h, uint16_t l )
{
  bit_t b;

  b.low  = l;
  b.high = h;

  return looks_like( a, b );
}

uint8_t
check_fs(airdata_t *d, airmsg_t *am)
{
  //DC('e');
  am->bytes = 0;
  if (d->bits<45) {
    // DU(d->bits,2); DNL();
    return 0;
  }

  uint8_t crc = 0;
  // read 5 bytes with parity ...
  for (uint8_t i=0;i<4;i++) {
    am->dec[i] = AD_POP( d, 8, 1 );
    if (parity_even_bit(am->dec[i]) != AD_SHL( d )) {
      // DS_P( PSTR("PE\r\n") );
      return 0;
    }
    crc += am->dec[i];
    am->bytes++;
  }

  // Extentions bit set?
  if (bit_is_set(am->dec[3],5)) {
    am->dec[4] = AD_POP( d, 8, 1 );
    if (parity_even_bit(am->dec[4]) != AD_SHL( d )) {
      // DS_P( PSTR("xPE\r\n") );
      return 0;
    }
    crc += am->dec[4];
    am->bytes++;
  }
  // Read crc
  uint8_t CRC = AD_POP( d, 8, 1 );
  if (parity_even_bit(CRC) != AD_SHL( d )) {
    // DS_P( PSTR("cPE\r\n") );
    return 0;
  }
  if (CRC-crc == 6) {
    am->type = 'F';
  } else if (CRC-crc == 12) {
    am->type = 'T';
  } else {
    // DS_P( PSTR("crc") ); DH(CRC-crc,2); DNL();
    return 0;
  }

  // DC(' ');DC(am->type); for (uint8_t i=0;i<am->bytes;i++) DH(am->dec[i], 2); DNL();

  return 1;
}

uint8_t
check_em(airdata_t *d, airmsg_t *am)
{
  // DC('e');
  am->bytes = 0;
  if (d->bits<89) {
    // DU(d->bits,2); DNL();
    return 0;
  }
  uint8_t crc = 0;
  // read 9 bytes with stopbit ...
  for (uint8_t i=0;i<9;i++) {
    am->dec[i] = AD_POP( d, 8, 0 );
    if (!AD_SHL( d )) {
      // DS_P( PSTR("eSB\r\n") );
      return 0;
    }
    crc = crc ^ am->dec[i];
    am->bytes++;
  }
  // Read crc
  uint8_t CRC = AD_POP( d, 8, 0 );
  if (CRC != crc) {
          // DS_P( PSTR("ecrc\r\n") );
          return 0;
  }
  am->type = 'E';
  return 1;
}

uint8_t
check_hms(airdata_t *d, airmsg_t *am) 
{
  // DC('m');
  am->bytes = 0;
  if (d->bits<69) {
          // DU(d->bits,2); DNL();
          return 0;
  }
  uint8_t crc = 0;
  // read 6 bytes with parity and stopbit...
  for (uint8_t i=0;i<6;i++) {
          am->dec[i] = AD_POP( d, 8, 0 );
          if (parity_even_bit(am->dec[i]) != AD_SHL( d )) {
                  // DS_P( PSTR("PE\r\n") );
                  return 0;
          }
          if (AD_SHL( d )) {
                  // DS_P( PSTR("SB\r\n") );
                  return 0;
          }
          crc = crc ^ am->dec[i];
          am->bytes++;
  }
  // Read crc
  uint8_t CRC = AD_POP( d, 8, 0 );
  if (parity_even_bit(CRC) != AD_SHL( d )) {
          // DS_P( PSTR("cPE\r\n") );
          return 0;
  }
  if (crc!=CRC) {
          // DS_P( PSTR("CRC\r\n") );
          return 0;
  }
  am->type = 'M';
  return 1;
}

uint8_t
check_ks(airdata_t *d, airmsg_t *am)
{
  // DC('k');
  am->bytes = 0;
  if (d->bits<35) {
          // DU(d->bits,2); DNL();
          return 0;
  }
  uint8_t crc = 0;
  uint8_t sum = 0;
  uint8_t i   = 0;
  uint8_t b   = 0;
  uint8_t B   = 0;

  // AD_DUMP( d, 20 );
  while (d->bits>4) {
    B = AD_POP( d, 4, 0 );
    crc = crc ^ B;
    sum = sum + B;
    if (i++) {
      am->dec[am->bytes++] = b | B;
      i = 0;
    } else { 
      b = B << 4;
    }
    if (!AD_SHL( d )) {
      // DS_P( PSTR("SB\r\n") );
      return 0;
    }
  }
  if(i)
    am->dec[am->bytes++] = b;
  if (crc) {
    // DS_P( PSTR("kcrc ") ); DU(crc,3);
    // DS_P( PSTR("sum ") ); DU(sum & 0x0f,3);
    // DNL();
    return 0; 
  }
  am->type = 'K';
  return 1;
}

uint8_t
analyse(airdata_t *d, airmsg_t *am)
{
  airdata_t D;

  memcpy( &D, d, sizeof( D ));
  if (check_fs( &D, am ))
    return 1;
  
  memcpy( &D, d, sizeof( D ));
  if (check_hms( &D, am ))
    return 1;

  memcpy( &D, d, sizeof( D ));
  if (check_ks( &D, am ))
    return 1;

  memcpy( &D, d, sizeof( D ));
  if (check_em( &D, am ))
    return 1;
  
  return 0;
}

/* --------------------------------------- ---------------------- */

void
tx_init(void)
{
  CC1100_OUT_DDR  |=  _BV(CC1100_OUT_PIN);
  CC1100_OUT_PORT &= ~_BV(CC1100_OUT_PIN);
  CC1100_IN_DDR   &= ~_BV(CC1100_IN_PIN);

  credit_10ms = MAX_CREDIT/2;

  TCCR1B = _BV(CS11);	// Timer1: 1us = 8MHz/8
  CLEAR_BIT( CC1100_IN_DDR, CC1100_IN_PIN );
  EICRB |= _BV(CC1100_ISC);  // Any edge of INTx generates an int.
  EIFR  |= _BV(CC1100_INT);  // clear  INT
  EIMSK |= _BV(CC1100_INT);  // enable INT
  TCNT1  = 0;
  OCR1B  = EDGETIMEOUT;	// Timeout
  TIMSK1 = 0;		// _BV( OCIE1A ) | _BV( OCIE1B ); // start disabled!

  cc_on = 0;
  bl = 0;
  bh = 0;
  memset( (void *)&ad, 0, sizeof( ad ));
}

void
set_txoff(void)
{
  ccStrobe(CC1100_SPWD);
  cc_on = 0;
}

void
set_txon(void)
{
  ccInitChip();
  cc_on = 1;
  ccRX();
}

void
set_txrestore(void)
{
  if(tx_report)
    set_txon();
}

void
set_txreport(char *in)
{
  fromhex(in+1, &tx_report, 1);
  tx_init();    // Sets up Counter1, needed by my_delay in ccReset

  if(tx_report) {
    set_txon();
  } else {
    set_txoff();
  }
}

static void
send_bit(uint8_t bit)
{
  CC1100_OUT_PORT |= _BV(CC1100_OUT_PIN);         // High
  my_delay_us(bit ? wait_high_one : wait_high_zero);

  CC1100_OUT_PORT &= ~_BV(CC1100_OUT_PIN);       // Low
  my_delay_us(bit ? wait_low_one : wait_low_zero);
}


// msg is with parity/checksum already added
static void
sendraw(uint8_t *msg, uint8_t nbyte, uint8_t bitoff,
                uint8_t repeat, uint8_t pause)
{
  // 12*800+1200+nbyte*(8*1000)+(bits*1000)+800+10000 
  // message len is < (nbyte+2)*repeat in 10ms units.
  int8_t i, j, sum = (nbyte+2)*repeat;
#ifndef BUSWARE_CUR
  if (credit_10ms < sum) {
    DS_P(PSTR("LOVF\r\n"));
    return;
  }
#endif
  credit_10ms -= sum;

  LED_ON();
  if(!cc_on) {
    tx_init();
    ccInitChip();
    cc_on = 1;
  }
  ccTX();                       // Enable TX 
  my_delay_ms(1);

  do {
    for(i = 0; i < 12; i++)                     // sync
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

  if(tx_report) {               // Enable RX
    ccRX();
  } else {
    ccStrobe(CC1100_SIDLE);
  }
  LED_OFF();
}

void
addParityAndSend(char *in, uint8_t startcs, uint8_t repeat)
{
  uint8_t hb[MAXMSG], hblen;
  hblen = fromhex(in+1, hb, MAXMSG-1);
  addParityAndSendData(hb, hblen, startcs, repeat);
}

static int
abit(uint8_t b, uint8_t obi)
{
  if(b)
    obuf[oby] |= _BV(obi);
  if(obi-- == 0) {
    if(oby < MAXMSG-1)
      oby++;
    obi = 7; obuf[oby] = 0;
  }
  return obi;
}

static uint8_t
cksum1(uint8_t s, uint8_t *buf, uint8_t len)    // FS20 / FHT
{
  while(len)
    s += buf[--len];
  return s;
}

void
addParityAndSendData(uint8_t *hb, uint8_t hblen,
                uint8_t startcs, uint8_t repeat)
{
  uint8_t iby;
  int8_t ibi, obi;

  hb[hblen] = cksum1(startcs, hb, hblen);
  hblen++;

  // Copy the message and add parity-bits
  iby=oby=0;
  ibi=obi=7;
  obuf[oby] = 0;

  while(iby<hblen) {
    obi = abit(hb[iby] & _BV(ibi), obi);
    if(ibi-- == 0) {
      obi = abit(parity_even_bit(hb[iby]), obi);
      ibi = 7; iby++;
    }
  }
  if(obi-- == 0) {                   // Trailing 0 bit: no need for a check
    oby++; obi = 7;
  }

  wait_high_zero = wait_low_zero = FS20_ZERO;
  wait_high_one  = wait_low_one  = FS20_ONE;
  sendraw(obuf, oby, obi, repeat, FS20_PAUSE);
}

void
fs20send(char *in)
{
  addParityAndSend(in, 6, 3);
}

void
rawsend(char *in)
{
  uint8_t hb[16];

  fromhex(in+1, hb, 16);
  wait_high_zero = hb[0]*10;
  wait_low_zero  = hb[1]*10;
  wait_high_one  = hb[2]*10;
  wait_low_one   = hb[3]*10;
  sendraw(hb+8, hb[6], 7-hb[7], hb[5], hb[4]);
}


////////////////////////////////////////////////////
// Receiver

TASK(RfAnalyze_Task)
{
  airdata_t d;
  airmsg_t am;

  if (adc==0)
    return;

  LED_ON();
  cli();

  memcpy( &d, &adq[0], sizeof( ad ) );
  adc--;
  memcpy( &adq[0], &adq[1], sizeof( ad )*adc );

  sei();

  if ( analyse( &d, &am ) ) {
    uint8_t isrep = 0;
    if(!(tx_report & REP_REPEATED)) {

      uint32_t msg_t = day * 86400 * 125 +
                       hour * 3600 * 125 +
                       minute + 60 * 125 +
                       sec * 125 +
                       hsec;

      if ( last_msg.type ) {
        if (memcmp(&am, &last_msg, sizeof(airmsg_t)) == 0 ) {
          if ( (msg_t - last_msg_t) <= 38 ) {
            isrep = 1;
          }
        }
      }
      last_msg_t = msg_t;
      memcpy(&last_msg, &am, sizeof(airmsg_t));
    }

    if(!isrep) {
      DC(am.type);
      for (uint8_t i=0;i<am.bytes;i++)
        DH(am.dec[i], 2);
      if(tx_report & REP_RSSI) {
        DC(' ');
        DH(cc1100_readReg(CC1100_RSSI),2);
        DC(' ');
      }
      if(tx_report & REP_BITS) {
        DS_P( PSTR(" raw output n/a anymore "));
      }
      DNL();
    }
  }

  LED_OFF();
}

//////////////////////////////////////////////////////////////////////
// Timer Compare Interrupt Handler. If we are called, then there was no
// data for SILENCE time, and we can analyze the data in the buffers

ISR(TIMER1_COMPB_vect)
{
  //   If we end up here we run into a timeout, no egdes anymore ...
  if (ad.state<100) {
    // RX
    if ((cc1100_readReg( CC1100_MARCSTATE ) & 0x1f) != 13) {
      ccStrobe( CC1100_SRX );
      TCNT1 = 0;
      OCR1B = 1000; // 1ms 
      TIFR1 |= _BV( OCF1B );
      //printf_P( PSTR("RX ") );
      return;
    }

    TIMSK1 = 0; // stop any timer interrupts
    SET_BIT( EIFR, CC1100_INT ); // clear  INT
    SET_BIT( EIMSK, CC1100_INT ); // enable INT
    OCR1B = EDGETIMEOUT;

    if (ad.bits>20 && adc<AD_Q_MX) {
      memcpy( (void*)&adq[adc], (void *)&ad, sizeof( ad ) );
      adc++;
    }
    bl = 0;
    bh = 0;
    memset( (void*)&ad, 0, sizeof( ad ));
  }
  TIFR1 |= _BV( OCF1B );
}

//////////////////////////////////////////////////////////////////////
// "Edge-Detected" Interrupt Handler
ISR(CC1100_INTVECT)
{
  if (ad.state == 99) { // HMS processing
    if(TCNT1<750)
      return;
    if(TCNT1>1250) {	 
      ad.state = 0;
    }
  }

  uint16_t c = TCNT1; // save the timer
  if (bit_is_set( PINC, CC1100_IN_PIN )) {
    // rising
    TCNT1 = 0;
    if (ad.state == 99) {
      AD_PUSH_0( &ad );
    } else if (bh) { // have full period covered?
      bl = c-bh;
      switch (ad.state) {
        case 1:	// collect SYNC bits of same kind
                // check if this bit is within bit window
          if ( looks_like_s( ad.bit[0], bh, bl) ) {
            // looks like the same 
            ad.sync++;
            ad.sum[0].high += bh;
            ad.sum[0].low  += bl;
            ad.bit[0].high = ad.sum[0].high / ad.sync;
            ad.bit[0].low  = ad.sum[0].low  / ad.sync;
            //DU(ad.bit[0].high,4);DC('/');DU(ad.bit[0].low,4);DNL();
            //DS_P( PSTR("0") );
            break;
          } else if (ad.sync>=16 && (ad.bit[0].high+ad.bit[0].low)>1600) {
            ad.state = 99;
            //DS_P( PSTR("HMS ") );
            break;
          } else if (ad.sync>=6) {
            // store signal for "1"
            ad.bit[1].high = bh;
            ad.bit[1].low  = bl;
            ad.sum[1].high = bh;
            ad.sum[1].low  = bl;
            ad.sync   = 1;

            ad.state  = 2;
            break;

          } else {
            ad.state  = 0;
          }

        case 2: // collect SYNC bits of same kind
                // check if this bit is within bit window
          if ( looks_like_s( ad.bit[0], bh, bl) ) {
            AD_PUSH_0( &ad );
            break;
          } else if ( looks_like_s( ad.bit[1], bh, bl) ) {
            AD_PUSH_1( &ad );
            if (ad.sync++<=6) {
                    ad.sum[1].high += bh;
                    ad.sum[1].low  += bl;
                    ad.bit[1].high = ad.sum[1].high / ad.sync;
                    ad.bit[1].low  = ad.sum[1].low  / ad.sync;
            }		
            break;

          } else {
            // printf_P( PSTR("?\r\n"));
            ad.state  = 0;
          }
        case 0: // INIT
          memset( (void *)&ad, 0, sizeof( ad ));
          ad.bit[0].high = bh;
          ad.bit[0].low  = bl;
          ad.sum[0].high = bh;
          ad.sum[0].low  = bl;
          ad.sync   = 1;
          ad.state  = 1;
          break;
      }
      bh = 0;
    }
  } else {

    // falling
    if (ad.state == 99) {
      TCNT1 = 0;
      AD_PUSH_1( &ad );
    } else {
      bh = c;
      bl = 0;
    }
  }
  TIFR1  |= _BV( OCF1A ) | _BV( OCF1B );	// clear Bittest and Timeout
  SET_BIT( TIMSK1, OCIE1B );
  LED_TOGGLE();
  EIFR  |= _BV(CC1100_INT);
}
