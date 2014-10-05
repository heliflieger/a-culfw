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
#include "clock.h"
#include "fncollection.h"
#include "fht.h"
#ifdef HAS_LCD
#include "pcf8833.h"
#endif
#include "fastrf.h"
#include "rf_router.h"

#ifdef HAS_ASKSIN
#include "rf_asksin.h"
#endif
#ifdef HAS_MBUS
#include "rf_mbus.h"
#endif

//////////////////////////
// With a CUL measured RF timings, in us, high/low sum
//           Bit zero        Bit one
//  KS300:  854/366 1220    366/854 1220
//  HRM:    992/448 1440    528/928 1456
//  EM:     400/320  720    432/784 1216
//  S300:   784/368 1152    304/864 1168
//  FHT:    362/368  730    565/586 1151
//  FS20:   376/357  733    592/578 1170
//  Revolt:  96/208  304    224/208  432


#define TSCALE(x)  (x/16)      // Scaling time to enable 8bit arithmetic
#define TDIFF      TSCALE(200) // tolerated diff to previous/avg high/low/total
#define SILENCE    4000        // End of message

#define STATE_RESET   0
#define STATE_INIT    1
#define STATE_SYNC    2
#define STATE_COLLECT 3
#define STATE_HMS     4
#define STATE_ESA     5
#define STATE_REVOLT  6
#define STATE_IT      7

uint8_t tx_report;              // global verbose / output-filter

typedef struct {
  uint8_t hightime, lowtime;
} wave_t;

// One bucket to collect the "raw" bits
typedef struct {
  uint8_t state, byteidx, sync, bitidx; 
  uint8_t data[MAXMSG];         // contains parity and checksum, but no sync
  wave_t zero, one; 
} bucket_t;

static bucket_t bucket_array[RCV_BUCKETS];
static uint8_t bucket_in;                 // Pointer to the in(terrupt) queue
static uint8_t bucket_out;                // Pointer to the out (analyze) queue
static uint8_t bucket_nrused;             // Number of unprocessed buckets
static uint8_t oby, obuf[MAXMSG], nibble; // parity-stripped output
static uint8_t roby, robuf[MAXMSG];       // for Repeat check: buffer and time
static uint32_t reptime;
#ifdef LONG_PULSE
static uint16_t hightime, lowtime;
#else
static uint8_t hightime, lowtime;
#endif

static void addbit(bucket_t *b, uint8_t bit);
static void delbit(bucket_t *b);
static uint8_t wave_equals(wave_t *a, uint8_t htime, uint8_t ltime);


void
tx_init(void)
{
  SET_BIT  ( CC1100_OUT_DDR,  CC1100_OUT_PIN);
  CLEAR_BIT( CC1100_OUT_PORT, CC1100_OUT_PIN);

  CLEAR_BIT( CC1100_IN_DDR,   CC1100_IN_PIN);
  SET_BIT( CC1100_EICR, CC1100_ISC);  // Any edge of INTx generates an int.

  credit_10ms = MAX_CREDIT/2;

  for(int i = 1; i < RCV_BUCKETS; i ++)
    bucket_array[i].state = STATE_RESET;
  cc_on = 0;
}

void
set_txrestore()
{
#ifdef HAS_MBUS	
  if(mbus_mode != WMBUS_NONE) {
    // rf_mbus.c handles cc1101 configuration on its own.
    // if mbus is activated the configuration must not be
    // changed here, that leads to a crash!
    return;
  }
#endif
  if(tx_report) {
    set_ccon();
    ccRX();

  } else {
    set_ccoff();

  }
}

void
set_txreport(char *in)
{
  if(in[1] == 0) {              // Report Value
    DH2(tx_report);
    DU(credit_10ms, 5);
    DNL();
    return;
  }

  fromhex(in+1, &tx_report, 1);
  set_txrestore();
}

////////////////////////////////////////////////////
// Receiver

uint8_t
cksum1(uint8_t s, uint8_t *buf, uint8_t len)    // FS20 / FHT
{
  while(len)
    s += buf[--len];
  return s;
}

uint8_t
cksum2(uint8_t *buf, uint8_t len)               // EM
{
  uint8_t s = 0;
  while(len)
    s ^= buf[--len];
  return s;
}

uint8_t
cksum3(uint8_t *buf, uint8_t len)               // KS300
{
  uint8_t x = 0, y = 5, cnt = 0;
  while(len) {
    uint8_t d = buf[--len];
    x ^= (d>>4);
    y += (d>>4);
    if(!nibble || cnt) {
      x ^= (d&0xf);
      y += (d&0xf);
    }
    cnt++;
  }
  y += x;
  return (y<<4)|x;
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

#ifdef HAS_ESA
uint8_t
analyze_esa(bucket_t *b)
{
  input_t in;
  in.byte = 0;
  in.bit = 7;
  in.data = b->data;

  oby = 0;

  if (b->state != STATE_ESA)
       return 0;

  if( (b->byteidx*8 + (7-b->bitidx)) != 144 )
       return 0;

  uint8_t salt = 0x89;
  uint16_t crc = 0xf00f;
  
  for (oby = 0; oby < 15; oby++) {
  
       uint8_t byte = getbits(&in, 8, 1);
     
       crc += byte;
    
       obuf[oby] = byte ^ salt;
       salt = byte + 0x24;
       
  }
  
  obuf[oby] = getbits(&in, 8, 1);
  crc += obuf[oby];
  obuf[oby++] ^= 0xff;

  crc -= (getbits(&in, 8, 1)<<8);
  crc -= getbits(&in, 8, 1);

  if (crc) 
       return 0;

  return 1;
}
#endif

#ifdef HAS_TX3
uint8_t
analyze_TX3(bucket_t *b)
{
  input_t in;
  in.byte = 0;
  in.bit = 7;
  in.data = b->data;
  uint8_t n, crc = 0;

  if(b->byteidx != 4 || b->bitidx != 1)
    return 0;

  for(oby = 0; oby < 4; oby++) {
    if(oby == 0) {
      n = 0x80 | getbits(&in, 7, 1);
    } else {
      n = getbits(&in, 8, 1);
    }
    crc = crc + (n>>4) + (n&0xf);
    obuf[oby] = n;
  }

  obuf[oby] = getbits(&in, 7, 1) << 1;
  crc = (crc + (obuf[oby]>>4)) & 0xF;
  oby++;

  if((crc >> 4) != 0 || (obuf[0]>>4) != 0xA)
    return 0;

  return 1;
}
#endif
#ifdef HAS_IT
uint8_t analyze_it(bucket_t *b)
{
  uint8_t i,j,itbit;
  if (b->byteidx != 3 || b->bitidx != 7 || b->state != STATE_IT)
    return 0;
  /*oby=0;
  obuf[oby]=0;
  for (i=0;i<3;i++) {
    for (j=0;j<8;j=j+2) {
      obuf[oby]=obuf[oby]<<1;
      itbit=(b->data[i]>>j)&3;
      if (itbit == 1) {
        obuf[oby] |= 1;
      } else if (itbit>1)
        return 0;
    }
    if (i==1) {
      oby++;
      obuf[oby]=0;
    }

  }
  nibble=1;oby++;
  */
  for (oby=0;oby<3;oby++)
    obuf[oby]=b->data[oby];
  return 1;
}
#endif
#ifdef HAS_REVOLT
uint8_t analyze_revolt(bucket_t *b)
{
  uint8_t sum=0;
  if (b->byteidx != 12 || b->state != STATE_REVOLT || b->bitidx != 0)
    return 0;
  for (oby=0;oby<11;oby++) {
    sum+=b->data[oby];
    obuf[oby]=b->data[oby];
  }
  if (sum!=b->data[11])
      return 0;
  return 1;
}
#endif
//////////////////////////////////////////////////////////////////////
void
RfAnalyze_Task(void)
{
  uint8_t datatype = 0;
  bucket_t *b;

  if(lowtime) {
    if(tx_report & REP_LCDMON) {
#ifdef HAS_LCD
      lcd_txmon(hightime, lowtime);
#else
      uint8_t rssi = cc1100_readReg(CC1100_RSSI);    //  0..256
      rssi = (rssi >= 128 ? rssi-128 : rssi+128);    // Swap
      if(rssi < 64)                                  // Drop low and high 25%
        rssi = 0;
      else if(rssi >= 192)
        rssi = 15;
      else 
        rssi = (rssi-80)>>3;
      DC('a'+rssi);
#endif
    }
    if(tx_report & REP_MONITOR) {
      DC('r'); if(tx_report & REP_BINTIME) DC(hightime);
      DC('f'); if(tx_report & REP_BINTIME) DC(lowtime);
    }
    lowtime = 0;
  }


  if(bucket_nrused == 0)
    return;

  LED_ON();

  b = bucket_array + bucket_out;

#ifdef HAS_IT
  if(!datatype && analyze_it(b))
    datatype = TYPE_IT;
#endif
#ifdef HAS_REVOLT
  if(!datatype && analyze_revolt(b))
    datatype = TYPE_REVOLT;
#endif
#ifdef LONG_PULSE
  if(b->state != STATE_REVOLT && b->state != STATE_IT) {
#endif
#ifdef HAS_ESA
  if(analyze_esa(b))
    datatype = TYPE_ESA;
#endif

  if(!datatype && analyze(b, TYPE_FS20)) {
    oby--;                                  // Separate the checksum byte
    uint8_t fs_csum = cksum1(6,obuf,oby);
    if(fs_csum == obuf[oby] && oby >= 4) {
      datatype = TYPE_FS20;

    } else if(fs_csum+1 == obuf[oby] && oby >= 4) {     // Repeater
      datatype = TYPE_FS20;
      obuf[oby] = fs_csum;                  // do not report if we get both

    } else if(cksum1(12, obuf, oby) == obuf[oby] && oby >= 4) {
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

#ifdef HAS_TX3
  if(!datatype && analyze_TX3(b))
    datatype = TYPE_TX3;
#endif

  if(!datatype) {
    // As there is no last rise, we have to add the last bit by hand
    addbit(b, wave_equals(&b->one, hightime, b->one.lowtime));
    if(analyze(b, TYPE_KS300)) {
      oby--;                                 
      if(cksum3(obuf, oby) == obuf[oby-nibble])
        datatype = TYPE_KS300;
    }
    if(!datatype)
      delbit(b);
  }

#ifdef HAS_HOERMANN
  // This protocol is not yet understood. It should be last in the row!
  if(!datatype && b->byteidx == 4 && b->bitidx == 4 &&
     wave_equals(&b->zero, TSCALE(960), TSCALE(480))) {

    addbit(b, wave_equals(&b->one, hightime, TSCALE(480)));
    for(oby=0; oby < 5; oby++)
      obuf[oby] = b->data[oby];
    datatype = TYPE_HRM;
  }
#endif
#ifdef LONG_PULSE
}
#endif
  if(datatype && (tx_report & REP_KNOWN)) {

    uint8_t isrep = 0;
    if(!(tx_report & REP_REPEATED)) {      // Filter repeated messages
      
      // compare the data
      if(roby == oby) {
        for(roby = 0; roby < oby; roby++)
          if(robuf[roby] != obuf[roby])
            break;

        if(roby == oby && (ticks - reptime < REPTIME)) // 38/125 = 0.3 sec
          isrep = 1;
      }

      // save the data
      for(roby = 0; roby < oby; roby++)
        robuf[roby] = obuf[roby];
      reptime = ticks;

    }

    if(datatype == TYPE_FHT && !(tx_report & REP_FHTPROTO) &&
       oby > 4 &&
       (obuf[2] == FHT_ACK        || obuf[2] == FHT_ACK2    ||
        obuf[2] == FHT_CAN_XMIT   || obuf[2] == FHT_CAN_RCV ||
        obuf[2] == FHT_START_XMIT || obuf[2] == FHT_END_XMIT ||
        (obuf[3] & 0x70) == 0x70))
      isrep = 1;

    if(!isrep) {
      DC(datatype);
      if(nibble)
        oby--;
      for(uint8_t i=0; i < oby; i++)
        DH2(obuf[i]);
      if(nibble)
        DH(obuf[oby]&0xf,1);
      if(tx_report & REP_RSSI)
        DH2(cc1100_readReg(CC1100_RSSI));
      DNL();
    }

  }



  if(tx_report & REP_BITS) {

    DC('p');
    DU(b->state,        2);
    DU(b->zero.hightime*16, 5);
    DU(b->zero.lowtime *16, 5);
    DU(b->one.hightime *16, 5);
    DU(b->one.lowtime  *16, 5);
    DU(b->sync,         3);
    DU(b->byteidx,      3);
    DU(7-b->bitidx,     2);
    DC(' ');
    if(tx_report & REP_RSSI) {
      DH2(cc1100_readReg(CC1100_RSSI));
      DC(' ');
    }
    if(b->bitidx != 7)
      b->byteidx++;

    for(uint8_t i=0; i < b->byteidx; i++)
       DH2(b->data[i]);
    DNL();

  }

  b->state = STATE_RESET;
  bucket_nrused--;
  bucket_out++;
  if(bucket_out == RCV_BUCKETS)
    bucket_out = 0;

  LED_OFF();

#ifdef HAS_FHT_80b
  if(datatype == TYPE_FHT) {
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
#ifdef LONG_PULSE
  uint16_t tmp;
#endif
  TIMSK1 = 0;                           // Disable "us"
#ifdef LONG_PULSE
  tmp=OCR1A;
  OCR1A = TWRAP;                        // Wrap Timer
  TCNT1=tmp;                            // reinitialize timer to measure times > SILENCE
#endif
  if(tx_report & REP_MONITOR)
    DC('.');

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

// Check for the validity of a 768:384us signal. Without PA ramping!
// Some CUL's generate 20% values wich are out of these bounderies.
uint8_t
check_rf_sync(uint8_t l, uint8_t s)
{
  return (l >= 0x25 &&          // 592
          l <= 0x3B &&          // 944
          s >= 0x0A &&          // 160
          s <= 0x26 &&          // 608
          l > s);
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
#ifdef HAS_FASTRF
  if(fastrf_on) {
    fastrf_on = 2;
    return;
  }
#endif

#ifdef HAS_RF_ROUTER
  if(rf_router_status == RF_ROUTER_DATA_WAIT) {
    rf_router_status = RF_ROUTER_GOT_DATA;
    return;
  }
#endif
#ifdef LONG_PULSE
  uint16_t c = (TCNT1>>4);               // catch the time and make it smaller
#else
  uint8_t c = (TCNT1>>4);               // catch the time and make it smaller
#endif

  bucket_t *b = bucket_array+bucket_in; // where to fill in the bit

  if (b->state == STATE_HMS) {
    if(c < TSCALE(750))
      return;
    if(c > TSCALE(1250)) {
      reset_input();
      return;
    }
  }

#ifdef HAS_ESA
  if (b->state == STATE_ESA) {
    if(c < TSCALE(375))
      return;
    if(c > TSCALE(625)) {
      reset_input();
      return;
    }
  }
#endif

  //////////////////
  // Falling edge
  if(!bit_is_set(CC1100_IN_PORT,CC1100_IN_PIN)) {
    if( (b->state == STATE_HMS)
#ifdef HAS_ESA
     || (b->state == STATE_ESA) 
#endif
    ) {
      addbit(b, 1);
      TCNT1 = 0;
    }
    hightime = c;
    return;

  }

  lowtime = c-hightime;
  TCNT1 = 0;                          // restart timer
  if( (b->state == STATE_HMS)
#ifdef HAS_ESA
     || (b->state == STATE_ESA) 
#endif
  ) {
    addbit(b, 0);
    return;
  }

  ///////////////////////
  // http://www.nongnu.org/avr-libc/user-manual/FAQ.html#faq_intbits
  TIFR1 = _BV(OCF1A);                 // clear Timers flags (?, important!)
  
#ifdef HAS_REVOLT
  if ((hightime > TSCALE(9000)) && (hightime < TSCALE(12000)) &&
      (lowtime  > TSCALE(150))   && (lowtime  < TSCALE(540))) {
    // Revolt
    b->zero.hightime = 6;
    b->zero.lowtime = 14;
    b->one.hightime = 19;
    b->one.lowtime = 14;
    b->sync=1;
    b->state = STATE_REVOLT;
    b->byteidx = 0;
    b->bitidx  = 7;
    b->data[0] = 0;
    OCR1A = SILENCE;
    TIMSK1 = _BV(OCIE1A);
    return;
  } 
#endif
#ifdef HAS_IT
  if ((hightime < TSCALE(500)) &&  (hightime > TSCALE(200)) &&
             (lowtime  < TSCALE(12160)) && (lowtime > TSCALE(9000))) {
    // Intertechno
    b->zero.hightime = hightime; 
    b->zero.lowtime = hightime*3+1;
    b->one.hightime = hightime*3+1;
    b->one.lowtime = hightime;
    b->sync=1;
    b->state = STATE_IT;
    b->byteidx = 0;
    b->bitidx  = 7;
    b->data[0] = 0;
    OCR1A = SILENCE;
    TIMSK1 = _BV(OCIE1A);
    return;
  }
#endif

  if(b->state == STATE_RESET) {   // first sync bit, cannot compare yet

retry_sync:
    if(hightime > TSCALE(1600) || lowtime > TSCALE(1600))
      return;
  
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

      OCR1A = SILENCE;
      if (b->sync >= 12 && (b->zero.hightime + b->zero.lowtime) > TSCALE(1600)) {
        b->state = STATE_HMS;

#ifdef HAS_ESA
      } else if (b->sync >= 10 && (b->zero.hightime + b->zero.lowtime) < TSCALE(600)) {
        b->state = STATE_ESA;
  
  OCR1A = 1000;
  
#endif
#ifdef HAS_RF_ROUTER
      } else if(rf_router_myid &&
                check_rf_sync(hightime, lowtime) &&
                check_rf_sync(b->zero.lowtime, b->zero.hightime)) {
        //display_channel=DISPLAY_USB;
        //DC('-');
        //display_channel=0xff;
        rf_router_status = RF_ROUTER_SYNC_RCVD;
        reset_input();
        return;
#endif

      } else {
        b->state = STATE_COLLECT;

      }

      b->one.hightime = hightime;
      b->one.lowtime  = lowtime;
      b->byteidx = 0;
      b->bitidx  = 7;
      b->data[0] = 0;

      TIMSK1 = _BV(OCIE1A);             // On timeout analyze the data

    } else {                            // too few sync bits

      b->state = STATE_RESET;
      goto retry_sync;

    }

  } else 
#ifdef HAS_REVOLT
  if (b->state==STATE_REVOLT) { //STATE_REVOLT

    if ((hightime < 11)) {
      addbit(b,0);
      b->zero.hightime = makeavg(b->zero.hightime, hightime);
      b->zero.lowtime  = makeavg(b->zero.lowtime,  lowtime);
    } else {
      addbit(b,1);
      b->one.hightime = makeavg(b->one.hightime, hightime);
      b->one.lowtime  = makeavg(b->one.lowtime,  lowtime);
    }
  } else 
#endif
    {                              // STATE_COLLECT, STATE_IT
    if(wave_equals(&b->one, hightime, lowtime)) {
      addbit(b, 1);
      b->one.hightime = makeavg(b->one.hightime, hightime);
      b->one.lowtime  = makeavg(b->one.lowtime,  lowtime);

    } else if(wave_equals(&b->zero, hightime, lowtime)) {
      addbit(b, 0);
      b->zero.hightime = makeavg(b->zero.hightime, hightime);
      b->zero.lowtime  = makeavg(b->zero.lowtime,  lowtime);

    } else {
      reset_input();

    }

  }

}

uint8_t
rf_isreceiving()
{
  return (bucket_array[bucket_in].state != STATE_RESET
#ifdef HAS_FHT_80b
        || fht80b_timeout != FHT_TIMER_DISABLED
#endif
          );
}
