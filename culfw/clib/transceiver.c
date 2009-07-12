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

#include "board.h"
#include "delay.h"
#include "transceiver.h"
#include "led.h"
#include "cc1100.h"
#include "display.h"
#include "clock.h"
#include "fncollection.h"
#include "fht.h"
#ifdef HAS_LCD
#include "pcf8833.h"
#endif


// For FS20 we time the complete message, for KS300 the rise-fall distance
// FS20  NULL: 400us high, 400us low
// FS20  ONE:  600us high, 600us low
// KS300 NULL  854us high, 366us low
// KS300 ONE:  366us high, 854us low

#define FS20_ZERO      400     //   400uS
#define FS20_ONE       600     //   600uS
#define FS20_PAUSE      10     // 10000mS

#define TSCALE(x)  (x/16)      // Scaling time to enable 8bit arithmetic
#define TDIFF      TSCALE(200) // tolerated diff to previous/avg high/low/total
#define SILENCE    4000        // End of message


#define STATE_RESET   0
#define STATE_INIT    1
#define STATE_SYNC    2
#define STATE_COLLECT 3
#define STATE_HMS     4

// For CUR request we need 
#define MAXMSG 17               // inclusive parity (Netto 15)

typedef struct {
  uint8_t hightime, lowtime;
} wave_t;

// One bucket to collect the "raw" bits
typedef struct {
  uint8_t state, byteidx, sync, bitidx; 
  uint8_t data[MAXMSG];         // contains parity and checksum, but no sync
  wave_t zero, one; 
} bucket_t;


uint16_t credit_10ms;
uint8_t tx_report;              // global verbose / output-filter


static bucket_t bucket_array[RCV_BUCKETS];
static uint8_t bucket_in;                 // Pointer to the in(terrupt) queue
static uint8_t bucket_out;                // Pointer to the out (analyze) queue
static uint8_t bucket_nrused;             // Number of unprocessed buckets
static uint8_t oby, obuf[MAXMSG], nibble; // parity-stripped output
static uint8_t roby, robuf[MAXMSG];       // for Repeat check: buffer and time
static uint32_t reptime;
static uint8_t cc_on;
static uint8_t hightime, lowtime;

static void send_bit(uint8_t bit);
static void sendraw(uint8_t msg[], uint8_t nbyte, uint8_t bitoff,
                    uint8_t repeat, uint8_t pause);
static uint8_t cksum1(uint8_t s, uint8_t *buf, uint8_t len);
static uint8_t cksum2(uint8_t *buf, uint8_t len);
static uint8_t cksum3(uint8_t *buf, uint8_t len);
static void reset_input(void);
static void addbit(bucket_t *b, uint8_t bit);
static void delbit(bucket_t *b);
static uint8_t wave_equals(wave_t *a, uint8_t htime, uint8_t ltime);

void
tx_init(void)
{
  CC1100_OUT_DDR  |=  _BV(CC1100_OUT_PIN);
  CC1100_OUT_PORT &= ~_BV(CC1100_OUT_PIN);
  CC1100_IN_DDR  &=  ~_BV(CC1100_IN_PIN);
  EICRB |= _BV(CC1100_ISC);                // Any edge of INTx generates an int.

  credit_10ms = MAX_CREDIT/2;

  for(int i = 1; i < RCV_BUCKETS; i ++)
    bucket_array[i].state = STATE_RESET;
  cc_on = 0;
}

void
set_txoff(void)
{
  ccStrobe(CC1100_SIDLE);
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
set_txrestore()
{
  tx_init();    // Sets up Counter1, needed by my_delay in ccReset
  if(tx_report) {
    set_txon();
  } else {
    set_txoff();
  }
}

void
set_txreport(char *in)
{
  if(in[1] == 0) {              // Report Value
    DH(tx_report, 2); DNL();
    return;
  }
  fromhex(in+1, &tx_report, 1);
  set_txrestore();
}

////////////////////////////////////////////////////
// Sender
static void
send_bit(uint8_t bit)
{
  CC1100_OUT_PORT |= _BV(CC1100_OUT_PIN);         // High
  my_delay_us(bit ? FS20_ONE : FS20_ZERO);

  CC1100_OUT_PORT &= ~_BV(CC1100_OUT_PIN);       // Low
  my_delay_us(bit ? FS20_ONE : FS20_ZERO);
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
  ccStrobe( CC1100_SIDLE );
  my_delay_ms(1);
  ccTX();                       // Enable TX 

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

  sendraw(obuf, oby, obi, repeat, FS20_PAUSE);
}

void
fs20send(char *in)
{
  addParityAndSend(in, 6, 3);
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
  uint8_t x = 0, cnt = 0;
  while(len) {
    uint8_t d = buf[--len];
    x ^= (d>>4);
    if(!nibble || cnt)
      x ^= (d&0xf);
    cnt++;
  }
  return x;
}

static uint8_t
analyze(bucket_t *b, uint8_t t)
{
  uint8_t cnt=0, max, iby = 0;
  int8_t ibi=7, obi=7;

  nibble = 0;
  oby = 0;
  max = b->byteidx*8+(7-b->bitidx);
  obuf[0] = 0;
  while(cnt++ < max) {

    uint8_t bit = (b->data[iby] & _BV(ibi)) ? 1 : 0;     // Input bit
    if(ibi-- == 0) {
      iby++;
      ibi=7;
    }

    if(t == TYPE_KS300 && obi == 3) {                   // nibble check
      if(!nibble) {
        if(!bit)
          return 0;
        nibble = !nibble;
        continue;
      }
      nibble = !nibble;
    }

    if(obi == -1) {                                    // next byte
      if(t == TYPE_FS20) {
        if(parity_even_bit(obuf[oby]) != bit)
          return 0;
      }
      if(t == TYPE_EM || t == TYPE_KS300) {
        if(!bit)
          return 0;
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
    return 0;
  else if(t == TYPE_EM && obi == -1)                  // missing last stopbit
    oby++;
  else if(nibble)                                     // half byte msg 
    oby++;
  if(oby == 0)
    return 0;
  return 1;
}

typedef struct  {
  uint8_t *data;
  uint8_t byte, bit;
} input_t;

uint8_t
getbit(input_t *in)
{
  uint8_t bit = (in->data[in->byte] & _BV(in->bit)) ? 1 : 0;
  if(in->bit-- == 0) {
    in->byte++;
    in->bit=7;
  }
  return bit;
}

uint8_t
getbits(input_t* in, uint8_t nbits, uint8_t msb)
{
  uint8_t ret = 0, i;
  for (i = 0; i < nbits; i++) {
    if (getbit(in) )
      ret = ret | _BV( msb ? nbits-i-1 : i );
  }
  return ret;
}

uint8_t
analyze_hms(bucket_t *b)
{
  input_t in;
  in.byte = 0;
  in.bit = 7;
  in.data = b->data;

  oby = 0;
  if(b->byteidx*8 + (7-b->bitidx) < 69) 
    return 0;

  uint8_t crc = 0;
  for(oby = 0; oby < 6; oby++) {
    obuf[oby] = getbits(&in, 8, 0);
    if(parity_even_bit(obuf[oby]) != getbit( &in ))
      return 0;
    if(getbit(&in))
      return 0;
    crc = crc ^ obuf[oby];
  }

  // Read crc
  uint8_t CRC = getbits(&in, 8, 0);
  if(parity_even_bit(CRC) != getbit(&in))
    return 0;
  if(crc!=CRC)
    return 0;
  return 1;
}


//////////////////////////////////////////////////////////////////////
// Timer Compare Interrupt Handler. If we are called, then there was no
// data for SILENCE time, and we can analyze the data in the buffers
TASK(RfAnalyze_Task)
{
  uint8_t datatype = 0;
  bucket_t *b;


  if(lowtime) {
#ifdef HAS_LCD
    if(tx_report & REP_LCDMON)
      lcd_txmon(hightime, lowtime);
#endif
    if(tx_report & REP_MONITOR) {
      DC('r'); if(tx_report & REP_BINTIME) DH(hightime,2);
      DC('f'); if(tx_report & REP_BINTIME) DH(lowtime, 2);
    }
    lowtime = 0;
  }


  if(bucket_nrused == 0)
    return;

  LED_ON();

  b = bucket_array + bucket_out;

  if(analyze(b, TYPE_FS20)) {
    oby--;                                  // Separate the checksum byte
    if(cksum1(6, obuf, oby) == obuf[oby]) {
      datatype = TYPE_FS20;
    } else if(cksum1(12, obuf, oby) == obuf[oby]) {
      datatype = TYPE_FHT;
    } else {
      datatype = 0;
    }
  }

  if(!datatype && analyze(b, TYPE_EM)) {
    oby--;                                 
    if(oby == 9 && cksum2(obuf, oby) == obuf[oby])
      datatype = TYPE_EM;
  }

  if(!datatype && analyze_hms(b))
    datatype = TYPE_HMS;

  if(!datatype) {
    // As there is no last rise, we have to add the last bit by hand
    addbit(b, wave_equals(&b->one, hightime, b->one.lowtime));
    if(analyze(b, TYPE_KS300)) {
      oby--;                                 
      if(cksum3(obuf, oby) == (obuf[oby-nibble]&0xf))
        datatype = TYPE_KS300;
    }
    if(!datatype)
      delbit(b);
  }

  // This protocol is not yet understood. It should be last in the row!
  if(!datatype && b->byteidx == 4 && b->bitidx == 4 &&
     wave_equals(&b->zero, TSCALE(960), TSCALE(480))) {

    addbit(b, wave_equals(&b->one, hightime, TSCALE(480)));
    oby = 0;
    obuf[oby] = b->data[oby]; oby++;
    obuf[oby] = b->data[oby]; oby++;
    obuf[oby] = b->data[oby]; oby++;
    obuf[oby] = b->data[oby]; oby++;
    obuf[oby] = b->data[oby]; oby++;
    datatype = TYPE_HRM;
  }


  if(datatype && (tx_report & REP_KNOWN)) {

    uint8_t isrep = 0;
    if(!(tx_report & REP_REPEATED)) {      // Filter repeated messages
      
      // compare the data

      uint32_t ctime = (((24*day + hour)*60 + minute)*60 + sec)*125+ hsec;

      if(roby == oby) {
        for(roby = 0; roby < oby; roby++)
          if(robuf[roby] != obuf[roby])
            break;

        if(roby == oby && ctime - reptime < 38) // 38/125 = 0.3 sec
          isrep = 1;
      }

      // save the data
      for(roby = 0; roby < oby; roby++)
        robuf[roby] = obuf[roby];
      reptime = ctime;

    }

    if(datatype == TYPE_FHT && !(tx_report & REP_FHTPROTO) &&
       (obuf[2] == FHT_ACK        || obuf[2] == FHT_ACK2    ||
        obuf[2] == FHT_CAN_XMIT   || obuf[2] == FHT_CAN_RCV ||
        obuf[2] == FHT_START_XMIT || obuf[2] == FHT_END_XMIT))
      isrep = 1;

    if(!isrep) {
      DC(datatype);
      if(nibble)
        oby--;
      for(uint8_t i=0; i < oby; i++)
        DH(obuf[i],2);
      if(nibble)
        DH(obuf[oby]&0xf,1);
      if(tx_report & REP_RSSI)
        DH(cc1100_readReg(CC1100_RSSI),2);
      DNL();
    }

  }


  if(tx_report & REP_BITS) {

    DC('p');
    DU(b->state,        2);
    DU(b->zero.hightime,4);
    DU(b->zero.lowtime, 4);
    DU(b->one.hightime, 4);
    DU(b->one.lowtime,  4);
    DU(b->sync,         3);
    DU(b->byteidx,      3);
    DU(7-b->bitidx,     2);
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

  b->state = STATE_RESET;
  bucket_nrused--;
  bucket_out++;
  if(bucket_out == RCV_BUCKETS)
    bucket_out = 0;

  LED_OFF();

#ifdef HAS_FHT_80b
  if(datatype == TYPE_FHT && oby == 5) {
    fht_hook(obuf);
  }
#endif
}

static void
reset_input(void)
{
  TIMSK1 = 0;
  bucket_array[bucket_in].state = STATE_RESET;
}

//////////////////////////////////////////////////////////////////////
// Timer Compare Interrupt Handler. If we are called, then there was no
// data for SILENCE time, and we can put the data to be analysed
ISR(TIMER1_COMPA_vect)
{
  TIMSK1 = 0;                           // Disable "us"

  if(bucket_array[bucket_in].state < STATE_COLLECT ||
     bucket_array[bucket_in].byteidx < 2) {    // false alarm
    reset_input();
    return;

  }

  if(bucket_nrused+1 == RCV_BUCKETS) {   // each bucket is full: reuse the last

    if(tx_report & REP_BITS)
      DS_P(PSTR("BOVF\r\n"));            // Bucket overflow

    reset_input();

  } else {

    bucket_nrused++;
    bucket_in++;
    if(bucket_in == RCV_BUCKETS)
      bucket_in = 0;

  }

}

static uint8_t
wave_equals(wave_t *a, uint8_t htime, uint8_t ltime)
{
  int16_t dlow = a->lowtime-ltime;
  int16_t dhigh = a->hightime-htime;
  int16_t dcomplete  = (a->lowtime+a->hightime) - (ltime+htime);
  if(dlow      < TDIFF && dlow      > -TDIFF &&
     dhigh     < TDIFF && dhigh     > -TDIFF &&
     dcomplete < TDIFF && dcomplete > -TDIFF)
    return 1;
  return 0;
}

uint8_t
makeavg(uint8_t i, uint8_t j)
{
  return (i+i+i+j)/4;
}

static void
addbit(bucket_t *b, uint8_t bit)
{
  if(b->byteidx>=sizeof(b->data)){
    reset_input();
    return;
  }
  if(bit)
    b->data[b->byteidx] |= _BV(b->bitidx);

  if(b->bitidx-- == 0) {           // next byte
    b->bitidx = 7;
    b->data[++b->byteidx] = 0;
  }
}

static void
delbit(bucket_t *b)
{
  if(b->bitidx++ == 7) {           // prev byte
    b->bitidx = 0;
    b->byteidx--;
  }
}

//////////////////////////////////////////////////////////////////////
// "Edge-Detected" Interrupt Handler
ISR(CC1100_INTVECT)
{
  uint8_t c = (TCNT1>>4);               // catch the time and make it smaller
  bucket_t *b = bucket_array+bucket_in; // where to fill in the bit

  if (b->state == STATE_HMS) {
    if(c < TSCALE(750))
      return;
    if(c > TSCALE(1250)) {
      reset_input();
      return;
    }
  }

  //////////////////
  // Falling edge
  if(!bit_is_set(CC1100_IN_PORT,CC1100_IN_PIN)) {
    if(b->state == STATE_HMS) {
      addbit(b, 1);
      TCNT1 = 0;
    }
    hightime = c;
    return;

  }

  lowtime = c-hightime;
  TCNT1 = 0;                          // restart timer
  if(b->state == STATE_HMS) {
    addbit(b, 0);
    return;
  }

  ///////////////////////
  // http://www.nongnu.org/avr-libc/user-manual/FAQ.html#faq_intbits
  TIFR1 = _BV(OCF1A);                 // clear Timers flags (?, important!)


  if(b->state == STATE_RESET) {   // first sync bit, cannot compare yet

    b->zero.hightime = hightime;
    b->zero.lowtime = lowtime;
    b->sync  = 1;
    b->state = STATE_SYNC;

  } else if(b->state == STATE_SYNC) {   // sync: lots of zeroes

    if(wave_equals(&b->zero, hightime, lowtime)) {

      b->zero.hightime = makeavg(b->zero.hightime, hightime);
      b->zero.lowtime  = makeavg(b->zero.lowtime,  lowtime);
      b->sync++;

    } else if(b->sync >= 4 ) {          // the one bit at the end of the 0-sync

      if (b->sync >= 12 &&
          (b->zero.hightime + b->zero.lowtime) > TSCALE(1600)) {
        b->state = STATE_HMS;

      } else {
        b->state = STATE_COLLECT;

      }

      b->one.hightime = hightime;
      b->one.lowtime  = lowtime;
      b->byteidx = 0;
      b->bitidx  = 7;
      b->data[0] = 0;

      OCR1A = SILENCE;
      TIMSK1 = _BV(OCIE1A);             // On timeout analyze the data

    } else {                            // too few sync bits

      reset_input();

    }

  } else {                              // STATE_COLLECT

    if(wave_equals(&b->one, hightime, lowtime)) {
      addbit(b, 1);
      if(b->byteidx == 0) {
        b->one.hightime = makeavg(b->one.hightime, hightime);
        b->one.lowtime  = makeavg(b->one.lowtime,  lowtime);
      }

    } else if(wave_equals(&b->zero, hightime, lowtime)) {
      addbit(b, 0);

    } else {
      reset_input();

    }

  }
}
