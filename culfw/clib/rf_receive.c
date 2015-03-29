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


#define STATE_INIT    1
#define STATE_SYNC    2
#define STATE_COLLECT 3


uint8_t tx_report;              // global verbose / output-filter

static uint8_t obuf[MAXMSG]; // parity-stripped output

static bucket_t bucket_array[RCV_BUCKETS];
static uint8_t bucket_in;                 // Pointer to the in(terrupt) queue
static uint8_t bucket_out;                // Pointer to the out (analyze) queue
static uint8_t bucket_nrused;             // Number of unprocessed buckets
static uint8_t nibble; // parity-stripped output
static uint8_t roby, robuf[MAXMSG];       // for Repeat check: buffer and time
static uint32_t reptime;


static void delbit(bucket_t *b);

uint8_t wave_equals(wave_t *a, uint8_t htime, uint8_t ltime, uint8_t state);


static pulse_t hightime, lowtime;

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
analyze(bucket_t *b, uint8_t t, uint8_t *oby)
{
  uint8_t i=0, cnt=0, max, iby = 0;
  int8_t ibi=7, obi=7;

  nibble = 0;
  
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
        if(parity_even_bit(obuf[i]) != bit)
          return 0;
      }
      if(t == TYPE_EM || t == TYPE_KS300) {
        if(!bit)
          return 0;
      }
      obuf[++i] = 0;
      obi = 7;

    } else {                                           // Normal bits
      if(bit) {
        if(t == TYPE_FS20)
          obuf[i] |= _BV(obi);
        if(t == TYPE_EM || t == TYPE_KS300)            // LSB
          obuf[i] |= _BV(7-obi);
      }
      obi--;
    }
  }
  if(cnt <= max)
    return 0;
  else if(t == TYPE_EM && obi == -1)                  // missing last stopbit
    i++;
  else if(nibble)                                     // half byte msg 
    i++;

  *oby = i;
  if(i == 0)
    return 0;
  return 1;
}


/*
 * Check for repeted message.
 * When Package is for e.g. IT or TCM, than there must be received two packages
 * with the same message. Otherwise the package are ignored.
 */
void checkForRepeatedPackage(uint8_t *datatype, bucket_t *b) {
#if defined (HAS_IT) || defined (HAS_TCM97001)
  if (*datatype == TYPE_IT || (*datatype == TYPE_TCM97001)) { 
      if (packetCheckValues.isrep == 1 && packetCheckValues.isnotrep == 0) { 
        packetCheckValues.isnotrep = 1;
        packetCheckValues.packageOK = 1;
      } else if (packetCheckValues.isrep == 1) {
        packetCheckValues.packageOK = 0;
      }
  } else {
#endif
      if (!packetCheckValues.isrep) {
        packetCheckValues.packageOK = 1;
      }
#if defined (HAS_IT) || defined (HAS_TCM97001)
  }
#endif
}



//////////////////////////////////////////////////////////////////////
void
RfAnalyze_Task(void)
{
  uint8_t datatype = 0;
  bucket_t *b;
  uint8_t oby = 0;

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

  analyze_intertechno(b, &datatype, obuf, &oby);
  analyze_tcm97001(b, &datatype, obuf, &oby);
  analyze_revolt(b, &datatype, obuf, &oby);
  analyze_esa(b, &datatype, obuf, &oby);
  analyze_hms(b, &datatype, obuf, &oby);

  if (b->state == STATE_COLLECT) {
    if(!datatype && analyze(b, TYPE_FS20, &oby)) { // Can be FS10 (433Mhz) or FS20 (868MHz)
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

    if(IS868MHZ && !datatype && analyze(b, TYPE_EM, &oby)) {
      oby--;                                 
      if(oby == 9 && cksum2(obuf, oby) == obuf[oby])
        datatype = TYPE_EM;
    }
    analyze_tx3(b, &datatype, obuf, &oby); // Can be 433Mhz or 868MHz
  }

    if(!datatype) {
      // As there is no last rise, we have to add the last bit by hand
      addbit(b, wave_equals(&b->one, hightime, b->one.lowtime, b->state));
      if(analyze(b, TYPE_KS300, &oby)) {
        oby--;                                 
        if(cksum3(obuf, oby) == obuf[oby-nibble])
          datatype = TYPE_KS300;
      }
      if(!datatype)
        delbit(b);
    }

#ifdef HAS_HOERMANN
    // This protocol is not yet understood. It should be last in the row!
    if(IS868MHZ && !datatype && b->byteidx == 4 && b->bitidx == 4 &&
       wave_equals(&b->zero, TSCALE(960), TSCALE(480), b->state)) {

      addbit(b, wave_equals(&b->one, hightime, TSCALE(480), b->state));
      for(oby=0; oby < 5; oby++)
        obuf[oby] = b->data[oby];
      datatype = TYPE_HRM;
    }
#endif

  if(datatype && (tx_report & REP_KNOWN)) {

    packetCheckValues.isrep = 0;
    packetCheckValues.packageOK = 0;
    //packetCheckValues.isnotrep = 0;
    if(!(tx_report & REP_REPEATED)) {      // Filter repeated messages
      
      // compare the data
      if(roby == oby) {
        for(roby = 0; roby < oby; roby++)
          if(robuf[roby] != obuf[roby]) {
            packetCheckValues.isnotrep = 0;
            break;
          }
        if(roby == oby && (ticks - reptime < REPTIME)) // 38/125 = 0.3 sec
          packetCheckValues.isrep = 1;
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
      packetCheckValues.isrep = 1;

    checkForRepeatedPackage(&datatype, b);

    if(packetCheckValues.packageOK) {
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
#ifdef DEBUG_SYNC
    DU(b->syncbit.hightime *16, 7);
    DU(b->syncbit.lowtime  *16, 7);
#endif
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
  if(IS868MHZ && datatype == TYPE_FHT) {
    fht_hook(obuf);
  }
#endif
}

void
reset_input(void)
{
  TIMSK1 = 0;
  bucket_array[bucket_in].state = STATE_RESET;
#if defined (HAS_IT) || defined (HAS_TCM97001)
  packetCheckValues.isnotrep = 0;
#endif
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

uint8_t wave_equals(wave_t *a, uint8_t htime, uint8_t ltime, uint8_t state)
{
  uint8_t tdiffVal = TDIFF;
#ifdef HAS_IT
  if(state == STATE_IT)
    tdiffVal = TDIFFIT;
#endif

  int16_t dlow = a->lowtime-ltime;
  int16_t dhigh = a->hightime-htime;
  int16_t dcomplete  = (a->lowtime+a->hightime) - (ltime+htime);
  if(dlow      < tdiffVal && dlow      > -tdiffVal &&
     dhigh     < tdiffVal && dhigh     > -tdiffVal &&
     dcomplete < tdiffVal && dcomplete > -tdiffVal)
    return 1;
  return 0;
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

  if (IS868MHZ && b->state == STATE_HMS) {
    if(c < TSCALE(750))
      return;
    if(c > TSCALE(1250)) {
      reset_input();
      return;
    }
  }

#ifdef HAS_ESA
  if (IS868MHZ && b->state == STATE_ESA) {
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
    if(IS868MHZ && ( (b->state == STATE_HMS)
#ifdef HAS_ESA
     || (IS868MHZ && b->state == STATE_ESA) 
#endif
    )) {
      addbit(b, 1);
      TCNT1 = 0;
    }
    hightime = c;
    return;
  }

  lowtime = c-hightime;
  TCNT1 = 0;                          // restart timer

  if (sync_intertechno(b, &hightime, &lowtime)) {
    return;
  }

  /*DU(hightime *16, 6);
  DU(lowtime*16, 6);
  DC('*');*/

  if(IS868MHZ && ((b->state == STATE_HMS)
#ifdef HAS_ESA
     || (b->state == STATE_ESA) 
#endif
  )) {
    addbit(b, 0);
    return;
  }

  ///////////////////////
  // http://www.nongnu.org/avr-libc/user-manual/FAQ.html#faq_intbits
  TIFR1 = _BV(OCF1A);                 // clear Timers flags (?, important!)
  
#ifdef HAS_REVOLT
  if (IS433MHZ && (hightime > TSCALE(9000)) && (hightime < TSCALE(12000)) &&
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
  switch(b->state)
  {
    case STATE_RESET: // first sync bit, cannot compare yet   
retry_sync:
      if (is_tcm97001(b, &hightime, &lowtime)) {
        return;
      }	else if (is_intertechno(b, &hightime, &lowtime)) {
        return;
      } else if(hightime > TSCALE(1600) || lowtime > TSCALE(1600)) {
        // Invalid Package
        return;
      }
     
      b->zero.hightime = hightime;
      b->zero.lowtime = lowtime;
      b->sync  = 1;
      b->state = STATE_SYNC;
      break;
    case STATE_SYNC:  // sync: lots of zeroes

      if(wave_equals(&b->zero, hightime, lowtime, b->state)) {

        

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
      break;
#ifdef HAS_REVOLT
    case STATE_REVOLT: //STATE_REVOLTT
      if ((hightime < 11)) {
        addbit(b,0);
        b->zero.hightime = makeavg(b->zero.hightime, hightime);
        b->zero.lowtime  = makeavg(b->zero.lowtime,  lowtime);
      } else {
        addbit(b,1);
        b->one.hightime = makeavg(b->one.hightime, hightime);
        b->one.lowtime  = makeavg(b->one.lowtime,  lowtime);
      }
      break;
#endif
#ifdef HAS_TCM97001
    case STATE_TCM97001:
      sync_tcm97001(b, &hightime, &lowtime);
      addbit_tcm97001(b, &hightime, &lowtime);
      break;
#endif
#ifdef HAS_IT
    case STATE_ITV3: 
      addbit_intertechno_v3(b, &hightime, &lowtime);
      break;
#endif
    default:
      if(wave_equals(&b->one, hightime, lowtime, b->state)) {
        addbit(b, 1);
        b->one.hightime = makeavg(b->one.hightime, hightime);
        b->one.lowtime  = makeavg(b->one.lowtime,  lowtime);
      } else if(wave_equals(&b->zero, hightime, lowtime, b->state)) {
        addbit(b, 0);
        b->zero.hightime = makeavg(b->zero.hightime, hightime);
        b->zero.lowtime  = makeavg(b->zero.lowtime,  lowtime);
      } else {
          if (b->state!=STATE_IT) 
            reset_input();
      }
      break;
  }

}

uint8_t
rf_isreceiving()
{
  uint8_t r = (bucket_array[bucket_in].state != STATE_RESET);
#ifdef HAS_FHT_80b
  r = (r || fht80b_timeout != FHT_TIMER_DISABLED);
#endif
  return r;
}

