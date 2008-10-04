/* 
 * Copyright by R.Koenig
 * Inspired by code from Alexander Neumann <alexander@bumpern.de>
 * License: GPL v2
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <avr/eeprom.h>
#include <util/parity.h>
#include <string.h>

#include "delay.h"
#include "transceiver.h"
#include "led.h"
#include "cc1100.h"
#include "display.h"
#include "clock.h"
#include "fncollection.h"


// For FS20 we time the complete message, for KS300 the rise-fall distance
// FS20  NULL: 400us high, 400us low
// FS20  ONE:  600us high, 600us low
// KS300 NULL  854us high, 366us low
// KS300 ONE:  366us high, 854us low

#define FS20_ZERO      400    // 400uS
#define FS20_ONE       600    // 600uS
#define FS20_PAUSE      10    // 10000uS =  10ms

#define TIMEDIFF       166    // tolerated diff to previous null value
#define TIMEDIFF_RISE  266    // FS20: min diff between one and null (1200-800)
#define TIMEDIFF_FALL -325    // KS300: min diff between one and null (366-854)
#define MINTIME_RISE   533    // 2/3 of minimum rise-rise: FS20 NULL
#define MAXTIME_RISE  1830    // 3/2 of maximum rise-rise: KS300 complete msg
#define MINTIME_FALL   100    // We have a problem timing falling edges
#define MAXTIME_FALL  1280    // 3/2 of maximum rise-fall: KS300 NULL
#define SILENCE  (4*MAXTIME_RISE)  // End of message

#define RISING_EDGE 0
#define FALLING_EDGE 1
#define STATE_RESET   0
#define STATE_INIT    1
#define STATE_SYNC    2
#define STATE_COLLECT 3


#define N_BUCKETS 4              // Must be even: a "rise" and a "fall" bucket
int16_t credit_10ms;
uint8_t tx_report;              // global verbose / output-filter
static bucket_t bucket_array[N_BUCKETS];
static uint8_t bucket_in, bucket_out, bucket_nrused;
static uint8_t oby, obuf[10];    // parity-stripped output
static uint8_t roby, robuf[10];  // For repeat check: last buffer and time
static uint8_t rday,rhour,rminute,rsec,rhsec;



static void fs20_send_zero(void);
static void fs20_send_one(void);
static void fs20_send_sync(void);
static inline void fs20_send_bit(uint8_t bit);
static inline void fs20_send_byte(uint8_t byte);
static uint8_t cksum1(uint8_t s, uint8_t *buf, uint8_t len);
static uint8_t cksum2(uint8_t *buf, uint8_t len);
static uint8_t cksum3(uint8_t *buf, uint8_t len);
static void reset_both_in_buckets(void);

void
tx_init(void)
{
  CC1100_CS_DDR  |=  _BV(CC1100_PINOUT);
  CC1100_CS_PORT &= ~_BV(CC1100_PINOUT);

  CLEAR_BIT( CC1100_CS_DDR, CC1100_PININ );// Want Input
  EICRB |= _BV(CC1100_ISC);                // Any edge of INTx generates an int.
  EIFR  |= _BV(CC1100_INTF);
  EIMSK |= _BV(CC1100_INT);

  // Timer1 
  TCCR1A = 0;
  TCCR1B = _BV(CS11) | _BV(WGM12); // 8MHz/8 -> 1MHz / 1us

  for(int i = 1; i < N_BUCKETS; i += 2) // falling buckets start at 1
    bucket_array[i].state = STATE_INIT;
}

void
set_txreport(char *in)
{
  fromhex(in+1, &tx_report, 1);

  tx_init();    // Sets up Counter1, needed by my_delay in ccReset

  if(tx_report) {

    if(eeprom_read_byte(EE_FACT_RESET) == 0xff) { // Factory reset
      eeprom_write_byte(EE_FACT_RESET, 0x00);
      cc_factory_reset();
    }

    ccInitChip();
    ccRX();

  } else {

    ccStrobe(CC1100_SIDLE);

  }
}

////////////////////////////////////////////////////
// Transmitter
void
fs20_send_zero(void)
{
  CC1100_CS_PORT |= _BV(CC1100_PINOUT);
  my_delay_us(FS20_ZERO);
  CC1100_CS_PORT &= ~_BV(CC1100_PINOUT);
  my_delay_us(FS20_ZERO);
}

void fs20_send_one(void)
{
  CC1100_CS_PORT |= _BV(CC1100_PINOUT);
  my_delay_us(FS20_ONE);
  CC1100_CS_PORT &= ~_BV(CC1100_PINOUT);
  my_delay_us(FS20_ONE);
}

void
fs20_send_sync(void)
{
  for(uint8_t i = 0; i < 12; i++)
      fs20_send_zero();
  fs20_send_one();
}

void
fs20_send_bit(uint8_t bit)
{
  if(bit > 0)
    fs20_send_one();
  else
    fs20_send_zero();
}

void
fs20_send_byte(uint8_t byte)
{
  uint8_t i = 7;
  do {
    fs20_send_bit(byte & _BV(i));
  } while (i-- > 0);

  fs20_send_bit(parity_even_bit(byte));
}

void
fs20_send(uint8_t csstart, uint8_t msg[], uint8_t msglen, uint8_t repeat)
{
  // (12*800+1200+8*(9*1000)+800+10000) = 93.6 msec to send (avg. value).
  //
  uint8_t sum = 9*repeat+repeat/2;
  if (credit_10ms < sum) {
    DS_P(PSTR("LOVF\r\n"));
    return;
  }
  credit_10ms -= sum;

  LED_ON();

  ccTX();                       // Enable TX

  do {
    fs20_send_sync();
    
    uint8_t sum = csstart;
    for(uint8_t j = 0; j < msglen; j++) {
      fs20_send_byte(msg[j]);
      sum += msg[j];
    }
    fs20_send_byte(sum);
    
    fs20_send_zero();
    
    my_delay_ms(FS20_PAUSE);
  } while(--repeat > 0);

  if(tx_report) {               // Enable RX
    ccRX();
  } else {
    ccStrobe(CC1100_SIDLE);
  }

  LED_OFF();
}

void
fs20send(char *in)
{
  uint8_t hb[5], hblen;
  hblen = fromhex(in+1, hb, 5);
  fs20_send(6, hb, hblen, 3);
}

void
fhtsend(char *in)
{
  uint8_t hb[5], hblen;
  hblen = fromhex(in+1, hb, 5);
  fs20_send(12, hb, hblen, 2);
}


////////////////////////////////////////////////////
// Receiver

static uint8_t
cksum1(uint8_t s, uint8_t *buf, uint8_t len)    // FS20 / FHT
{
  while(len)
    s += buf[--len];
  return s;
}

static uint8_t
cksum2(uint8_t *buf, uint8_t len)               // EM
{
  uint8_t s = 0;
  while(len)
    s ^= buf[--len];
  return s;
}

static uint8_t
cksum3(uint8_t *buf, uint8_t len)               // KS300
{
  uint8_t x = 0;
  while(len) {
    uint8_t d = buf[--len];
    x ^= (d>>4);
    x ^= (d&0xf);
  }
  return x;
}


static uint8_t
analyze(bucket_t *b, uint8_t t)
{
  uint8_t cnt=0, isok = 1, max, iby = 0, nibble=1;
  int8_t ibi=7, obi=7;

  oby = 0;
  max = b->byteidx*8+(7-b->bitidx);
  obuf[0] = 0;
  while(cnt++ < max) {
    uint8_t bit = (b->data[iby] & _BV(ibi)) ? 1 : 0;     // Input bit
    if(ibi-- == 0) {
      iby++;
      ibi=7;
    }

    if(t == TYPE_KS300 && obi == 3) {                           // nibble check
      if(nibble) {
        if(!bit) {
          isok = 0;
          break;
        }
        nibble = !nibble;
        continue;
      }
      nibble = !nibble;
    }

    if(obi == -1) {                                    // next byte
      if(t == TYPE_FS20) {
        if(parity_even_bit(obuf[oby]) != bit) {
          isok = 0;
          break;
        }
      }
      if(t == TYPE_EM || t == TYPE_KS300) {
        if(!bit) {
          isok = 0;
          break;
        }
      }
      obuf[++oby] = 0;
      obi = 7;

    } else {                                           // Normal bits
      if(bit) {
        if(t == TYPE_FS20)
          obuf[oby] |= _BV(obi);
        if(t == TYPE_EM || t == TYPE_KS300)            // LSB
          obuf[oby] |= _BV(7-obi);
      }
      obi--;
    }
  }
  if(cnt <= max)
    isok = 0;
  else if(isok && t == TYPE_EM && obi == -1)           // missing last stopbit
    oby++;
  if(oby == 0)
    isok = 0;
  return isok;
}


//////////////////////////////////////////////////////////////////////
// Timer Compare Interrupt Handler. If we are called, then there was no
// data for SILENCE time, and we can analyze the data in the buffers
TASK(RfAnalyze_Task)
{
  uint8_t datatype = 0;
  bucket_t *rb, *fb;            // Rising and falling bucket pointer

  if(bucket_nrused == 0)
    return;

  LED_ON();

  rb = bucket_array + bucket_out;
  fb = rb + 1;

  if(rb->state == STATE_COLLECT) {

    if(analyze(rb, TYPE_FS20)) {
      oby--;                                  // Separate the checksum byte
      if(cksum1(6, obuf, oby) == obuf[oby]) {
        datatype = TYPE_FS20;
      } else if(cksum1(12, obuf, oby) == obuf[oby]) {
        datatype = TYPE_FHT;
      } else {
        datatype = 0;
      }
    }

    if(!datatype && analyze(rb, TYPE_EM)) {
      oby--;                                 
      if(oby == 9 && cksum2(obuf, oby) == obuf[oby])
        datatype = TYPE_EM;
    }

  }

  if(!datatype && fb->state == STATE_COLLECT) {
    if(analyze(fb, TYPE_KS300)) {
      oby--;                                 
      if(cksum3(obuf, oby) == (obuf[oby]&0xf))
        datatype = TYPE_KS300;
    }

    // This protocol is not yet understood
    if(!datatype && fb->byteidx == 4 && fb->bitidx == 4) {
      oby = 0;
      obuf[oby] = fb->data[oby]; oby++;
      obuf[oby] = fb->data[oby]; oby++;
      obuf[oby] = fb->data[oby]; oby++;
      obuf[oby] = fb->data[oby]; oby++;
      obuf[oby] = fb->data[oby]; oby++;
      datatype = TYPE_HRM;
    }
  }


  if(datatype && (tx_report & REP_KNOWN)) {

    uint8_t isrep = 0;
    if(!(tx_report & REP_REPEATED)) {      // Filter repeated messages
      
      // compare the data
      if(roby == oby) {
        for(roby = 0; roby < oby; roby++)
          if(robuf[roby] != obuf[roby])
            break;

        if(roby == oby) {       // data is equal, substract time
          uint16_t diff;
          diff = (day-rday)*24+(hour-rhour);
          if(diff <= 1) {
            diff = diff*3600+(minute-rminute)*60+(sec-rsec);
            if(diff <= 1) {
              diff = diff*125+(hsec-rhsec);
              if(diff <= 38)    // 38/125 = 0.3 sec
                isrep = 1;
            }
          }
        }
      }

      // save the data
      for(roby = 0; roby < oby; roby++)
        robuf[roby] = obuf[roby];
      rday=day; rhour=hour; rminute=minute; rsec=sec; rhsec=hsec;
    }

    if(!isrep) {
      DC(datatype);
      for(uint8_t i=0; i < oby; i++)
        DH(obuf[i],2);
      if(tx_report & REP_RSSI)
        DH(cc1100_readReg(CC1100_RSSI),2);
      DNL();
    }

  }


  if(tx_report & REP_BITS) {

    bucket_t *b = (rb->state == STATE_COLLECT ? rb : fb);

    DC('p');
    display_hex((rb->state<<4)|fb->state, 3, ' ');
    DU(b->zero, 5);
    DU(b->avg,  5);
    DU(b->sync, 3);
    DU(b->byteidx, 3);
    DU(7-b->bitidx, 2);
    DC(' ');
    if(tx_report & REP_RSSI) {
      DH(cc1100_readReg(CC1100_RSSI),2);
      DC(' ');
    }
    if(b->bitidx != 7)
      b->byteidx++;

    for(uint8_t i=0; i < b->byteidx; i++)
       DH(b->data[i],2);
    DNL();
  }

  rb->state = STATE_RESET;
  fb->state = STATE_INIT;

  bucket_nrused -= 2;
  bucket_out += 2;
  if(bucket_out == N_BUCKETS)
    bucket_out = 0;

  LED_OFF();
}

static void
reset_both_in_buckets(void)
{
  TIMSK1 = 0;
  bucket_array[bucket_in  ].state = STATE_RESET;
  bucket_array[bucket_in+1].state = STATE_INIT;
}

//////////////////////////////////////////////////////////////////////
// Timer Compare Interrupt Handler. If we are called, then there was no
// data for SILENCE time, and we can put the data to be analysed
ISR(TIMER1_COMPA_vect)
{
  TIMSK1 = 0;                           // Disable "us"

  if(bucket_array[bucket_in  ].state != STATE_COLLECT &&    // false alarm
     bucket_array[bucket_in+1].state != STATE_COLLECT) {
    reset_both_in_buckets();
    return;

  }

  if(bucket_nrused+2 == N_BUCKETS) {     // each bucket is full: reuse the last

    if(tx_report & REP_BITS)
      DS_P(PSTR("BOVF\r\n"));

    reset_both_in_buckets();

  } else {

    bucket_nrused += 2;
    bucket_in += 2;
    if(bucket_in == N_BUCKETS)
      bucket_in = 0;

  }

}

//////////////////////////////////////////////////////////////////////
// "Edge-Detected" Interrupt Handler
ISR(CC1100_INTVECT)
{
  uint8_t  rf;                          // 0: rise, 1: fall
  bucket_t *b;                          // where to fill in the bit
  uint16_t c = TCNT1;                   // catch the time!

  if(bit_is_set(CC1100_PINGRP,CC1100_PININ)) {
    TCNT1 = 0;                          // restart timer
    TIFR1 |= _BV(OCF1A);                // clear Timers flags (?, important!)
    rf = RISING_EDGE;
  } else {
    rf = FALLING_EDGE;
  }
  b = bucket_array+bucket_in+rf;        // HACK: falling / rising bucket

  if(tx_report & REP_MONITOR) {
    DC((rf==RISING_EDGE ? 'r' : 'f') + b->state);
    if(tx_report & REP_BINTIME) {
      DC((c>>8) & 0xff);
      DC( c     & 0xff);
    }
    if(!(tx_report & ~(REP_MONITOR|REP_BINTIME)) ) // ignore the rest
      return;
  }


  if(b->state == STATE_RESET) {  // Rise: timer is reset, start timing

    b->state = STATE_INIT;

  } else if((rf==RISING_EDGE  && (c<MINTIME_RISE||c>MAXTIME_RISE)) ||
            (rf==FALLING_EDGE && (c<MINTIME_FALL||c>MAXTIME_FALL))) {

    reset_both_in_buckets();

  } else if(b->state == STATE_INIT) {   // first sync bit, cannot compare yet

    b->zero  = c;
    b->sync  = 1;
    b->state = STATE_SYNC;

  } else if(b->state == STATE_SYNC) {   // sync: lots of zeroes

    int16_t d = c - b->zero;

    if (-TIMEDIFF < d && d < TIMEDIFF) {// looks like an additional zero

      b->sync++;                        // (3*z+c)/4 is cheaper then sum/sync
      b->zero = (b->zero+b->zero+b->zero+c)/4;

    } else if (b->sync>=6 && ((rf==RISING_EDGE  && d>TIMEDIFF_RISE) ||
                              (rf==FALLING_EDGE && d<TIMEDIFF_FALL))) {
                   
      b->avg = (b->zero+c)/2;           // avarage between 0 and 1
      b->byteidx = 0;
      b->bitidx  = 7;
      b->data[0] = 0;
      b->state = STATE_COLLECT;
      OCR1A = SILENCE;
      TIMSK1 = _BV(OCIE1A);             // On timeout analyze the data

    } else {

      // We are here if abs(diff) > 166 and:
      // RISING_EDGE:  d < 266.  Ideal FS20: 400, KS300: 0
      // FALLING_EDGE: d >-325.  Ideal FS20: 200, KS300: -488
      if(rf == RISING_EDGE)
        reset_both_in_buckets();
      else
        b->state = STATE_INIT;

    }

  } else {                              // STATE_COLLECT

    if(b->byteidx>=sizeof(b->data)) {

      reset_both_in_buckets();

    } else {

      if((rf==RISING_EDGE && c > b->avg) || (rf==FALLING_EDGE && c < b->avg))
        b->data[b->byteidx] |= _BV(b->bitidx);

      if(b->bitidx-- == 0) {           // next byte
        b->bitidx = 7;
        b->data[++b->byteidx] = 0;
      }

    }
  }
}
