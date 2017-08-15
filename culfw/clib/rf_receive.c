/* 
 * Copyright by R.Koenig
 * Copyright 2015 B. Hempel 
 * Inspired by code from Dirk Tostmann
 *
 * License: 
 *
 * This program is free software; you can redistribute it and/or modify it under 
 * the terms of the GNU General Public License as published by the Free Software 
 * Foundation; either version 2 of the License, or (at your option) any later 
 * version.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for 
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with 
 * this program; if not, write to the 
 * Free Software Foundation, Inc., 
 * 51 Franklin St, Fifth Floor, Boston, MA 02110, USA
 *
 */
#include <avr/interrupt.h>              // for ISR
#include <avr/pgmspace.h>               // for PSTR
#include <stdbool.h>                    // for bool, false
#include <stdint.h>                     // for int8_t
#include <util/parity.h>                // for parity_even_bit

#include "board.h"                      // for HAS_HMS, HAS_ESA, etc
#include "cc1100.h"                     // for cc1100_readReg, CC1100_RSSI, etc
#include "clock.h"                      // for ticks
#include "display.h"                    // for DU, DC, DH2, DNL, DH, DS_P
#include "fband.h"                      // for IS868MHZ, IS433MHZ
#include "fht.h"                        // for fht_hook, FHT_ACK, FHT_ACK2, etc
#include "led.h"                        // for CLEAR_BIT, SET_BIT, LED_OFF, etc
#include "rf_receive.h"
#include "rf_receive_bucket.h"          // for bucket_t, wave_t, TSCALE, etc
#include "rf_receive_esa.h"             // for STATE_ESA, analyze_esa
#include "rf_receive_hms.h"             // for STATE_HMS, analyze_hms
#include "rf_receive_it.h"              // for analyze_intertechno, etc
#include "rf_receive_revolt.h"          // for addbit_revolt, etc
#include "rf_receive_tcm97001.h"        // for analyze_tcm97001, etc
#include "rf_receive_tx3.h"             // for analyze_tx3
#include "rf_send.h"                    // for credit_10ms, MAX_CREDIT
#include "stringfunc.h"                 // for fromhex
#ifdef HAS_LCD
#include "pcf8833.h"                    // for lcd_txmon
#endif
#include "fastrf.h"                     // for fastrf_on
#include "rf_router.h"                  // for rf_router_status, etc
#ifdef HAS_MBUS
#include "rf_mbus.h"                    // for WMBUS_NONE, mbus_mode
#endif
#include "rf_mode.h"
#include "multi_CC.h"

#ifdef USE_HAL
#include "hal.h"
#endif

#ifndef USE_RF_MODE
uint8_t tx_report;              // global verbose / output-filter
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

static uint8_t obuf[MAXMSG]; // parity-stripped output

static bucket_t bucket_array[NUM_SLOWRF][RCV_BUCKETS];
static uint8_t bucket_in[NUM_SLOWRF];                 // Pointer to the in(terrupt) queue
static uint8_t bucket_out[NUM_SLOWRF];                // Pointer to the out (analyze) queue
static uint8_t bucket_nrused[NUM_SLOWRF];             // Number of unprocessed buckets
static uint8_t nibble; // parity-stripped output
static uint8_t roby, robuf[MAXMSG];       // for Repeat check: buffer and time
static uint32_t reptime[NUM_SLOWRF];
static uint16_t maxLevel[NUM_SLOWRF];


static void delbit(bucket_t *b);

uint8_t wave_equals(wave_t *a, uint8_t htime, uint8_t ltime, uint8_t state);


static pulse_t hightime[NUM_SLOWRF], lowtime[NUM_SLOWRF];

void
tx_init(void)
{
#ifdef USE_RF_MODE
  init_RF_mode();
#endif

#ifdef USE_HAL
#ifdef HAS_MULTI_CC
  for(uint8_t x=0; x < HAS_MULTI_CC; x++) {
    hal_CC_GDO_init(x,INIT_MODE_IN_CS_IN);
  }
#else
  hal_CC_GDO_init(0,INIT_MODE_OUT_CS_IN);
#endif

#else
  SET_BIT  ( CC1100_OUT_DDR,  CC1100_OUT_PIN);
  CLEAR_BIT( CC1100_OUT_PORT, CC1100_OUT_PIN);

  CLEAR_BIT( CC1100_IN_DDR,   CC1100_IN_PIN);
  SET_BIT( CC1100_EICR, CC1100_ISC);  // Any edge of INTx generates an int.
#endif

  credit_10ms = MAX_CREDIT/2;

  for(int i = 1; i < RCV_BUCKETS; i ++)
    bucket_array[CC_INSTANCE][i].state = STATE_RESET;
#ifndef USE_RF_MODE
  cc_on = 0;
#endif
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

  if(TX_REPORT) {
    set_ccon();
#ifdef HAS_MULTI_CC
    if(CC1101.instance < NUM_SLOWRF)
#endif
      ccRX();

  } else {
    set_ccoff();

  }
}

void
set_txreport(char *in)
{
  if(in[1] == 0) {              // Report Value
    MULTICC_PREFIX();
    DH2(TX_REPORT);
    DU(credit_10ms, 5);
    DNL();
    return;
  }

  fromhex(in+1, &TX_REPORT, 1);

#ifdef USE_RF_MODE
  set_RF_mode(RF_mode_slow);
#else
  set_txrestore();
#endif
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
#if defined (HAS_IT) && defined (HAS_TCM97001)
  if ((*datatype == TYPE_IT) || (*datatype == TYPE_TCM97001)) { 
#elif defined (HAS_TCM97001)
  if (*datatype == TYPE_TCM97001) {
#elif defined (HAS_IT)
  if (*datatype == TYPE_IT) {
#endif
#if defined (HAS_IT) || defined (HAS_TCM97001)
      if (packetCheckValues.isrep == 1 && packetCheckValues.isnotrep == 0) { 
        packetCheckValues.isnotrep = 1;
        packetCheckValues.packageOK = 1;
      } else if (packetCheckValues.isrep == 1) {
        packetCheckValues.packageOK = 0;
      } else {
        packetCheckValues.isnotrep = 0;
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

#ifdef HAS_MANCHESTER
static void analyseMC(bucket_t *b, uint8_t *datatype, uint8_t *obuf, uint8_t *oby) {    
    if (IS433MHZ && *datatype == 0) {
        if ((b->state == STATE_MC)) {
	        uint8_t i;
	        for (i=0;i<b->byteidx;i++) {
	          obuf[i]=b->data[i];
	        }
	        if (7-b->bitidx != 0) {
		        obuf[i]=b->data[i];
		        i++;
	        }
	        *oby = i;
	        *datatype = TYPE_MC;
	    }
    }
}
#endif

//////////////////////////////////////////////////////////////////////
void
RfAnalyze_Task(void)
{
  uint8_t datatype = 0;
  bucket_t *b;
  uint8_t oby = 0;

  if(lowtime[CC_INSTANCE]) {
#ifndef NO_RF_DEBUG
    if(TX_REPORT & REP_LCDMON) {
#ifdef HAS_LCD
      lcd_txmon(hightime[CC_INSTANCE], lowtime[CC_INSTANCE]);
#else
      uint8_t rssi = cc1100_readReg(CC1100_RSSI);    //  0..256
      rssi = (rssi >= 128 ? rssi-128 : rssi+128);    // Swap
      if(rssi < 64)                                  // Drop low and high 25%
        rssi = 0;
      else if(rssi >= 192)
        rssi = 15;
      else 
        rssi = (rssi-80)>>3;
      MULTICC_PREFIX();
      DC('a'+rssi);
#endif
    }
    if(TX_REPORT & REP_MONITOR) {
      DC('r'); if(TX_REPORT & REP_BINTIME) DC(hightime[CC_INSTANCE]);
      DC('f'); if(TX_REPORT & REP_BINTIME) DC(lowtime[CC_INSTANCE]);
    }
#endif // NO_RF_DEBUG
    lowtime[CC_INSTANCE] = 0;
  }


  if(bucket_nrused[CC_INSTANCE] == 0)
    return;

  LED_ON();

  b = bucket_array[CC_INSTANCE] + bucket_out[CC_INSTANCE];

#ifdef HAS_IT
  analyze_intertechno(b, &datatype, obuf, &oby);
#endif
#ifdef HAS_TCM97001
  analyze_tcm97001(b, &datatype, obuf, &oby);
#endif
#ifdef HAS_REVOLT
  analyze_revolt(b, &datatype, obuf, &oby);
#endif
#ifdef HAS_ESA
  analyze_esa(b, &datatype, obuf, &oby);
#endif
#ifdef HAS_HMS
  analyze_hms(b, &datatype, obuf, &oby);
#endif
#ifdef HAS_MANCHESTER
    analyseMC(b, &datatype, obuf, &oby);
#endif
#if defined(HAS_IT) || defined(HAS_TCM97001) 
  if(datatype == 0 && b->state == STATE_SYNC_PACKAGE) {
    // unknown sync package
    uint8_t ix;
    for (ix=0;ix<b->byteidx;ix++)
      obuf[ix]=b->data[ix];

    if (7-b->bitidx != 0) {
      obuf[ix]=b->data[ix];
      ix++; 
    }    
    oby = ix;
  }
#endif

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

    if(!datatype) {
      // As there is no last rise, we have to add the last bit by hand
      addbit(b, wave_equals(&b->one, hightime[CC_INSTANCE], b->one.lowtime, b->state));
      if(analyze(b, TYPE_KS300, &oby)) {
        oby--;                                 
        if(cksum3(obuf, oby) == obuf[oby-nibble])
          datatype = TYPE_KS300;
      }
      if(!datatype)
        delbit(b);
    }
  }

#ifdef HAS_HOERMANN
    // This protocol is not yet understood. It should be last in the row!
    if(IS868MHZ && !datatype && b->byteidx == 4 && b->bitidx == 4 &&
       wave_equals(&b->zero, TSCALE(960), TSCALE(480), b->state)) {

      addbit(b, wave_equals(&b->one, hightime[CC_INSTANCE], TSCALE(480), b->state));
      for(oby=0; oby < 5; oby++)
        obuf[oby] = b->data[oby];
      datatype = TYPE_HRM;
    }
#endif

  if(datatype && (TX_REPORT & REP_KNOWN)) {

    packetCheckValues.isrep = 0;
    packetCheckValues.packageOK = 0;
    //packetCheckValues.isnotrep = 0;
    if(!(TX_REPORT & REP_REPEATED)) {      // Filter repeated messages
      
      // compare the data
      if(roby == oby) {
        for(roby = 0; roby < oby; roby++)
          if(robuf[roby] != obuf[roby]) {
            packetCheckValues.isnotrep = 0;
            break;
          }
        if(roby == oby && (ticks - reptime[CC_INSTANCE] < REPTIME)) // 38/125 = 0.3 sec
          packetCheckValues.isrep = 1;
      }

      // save the data
      for(roby = 0; roby < oby; roby++)
        robuf[roby] = obuf[roby];
      reptime[CC_INSTANCE] = ticks;

    }

    if(datatype == TYPE_FHT && !(TX_REPORT & REP_FHTPROTO) &&
       oby > 4 &&
       (obuf[2] == FHT_ACK        || obuf[2] == FHT_ACK2    ||
        obuf[2] == FHT_CAN_XMIT   || obuf[2] == FHT_CAN_RCV ||
        obuf[2] == FHT_START_XMIT || obuf[2] == FHT_END_XMIT ||
        (obuf[3] & 0x70) == 0x70))
      packetCheckValues.isrep = 1;

    checkForRepeatedPackage(&datatype, b);

#if defined(HAS_RF_ROUTER) && defined(HAS_FHT_80b)
    if(datatype == TYPE_FHT && rf_router_target && !fht_hc0) // Forum #50756
      packetCheckValues.packageOK = 0;
#endif

    if(packetCheckValues.packageOK) {
      MULTICC_PREFIX();
      DC(datatype);
#ifdef HAS_HOMEEASY
      if (b->state == STATE_HE) {
        DC('h');
      }
#endif
#ifdef HAS_MANCHESTER
    if (b->state == STATE_MC) {
		  DC('m');
	}
#endif
      if(nibble)
        oby--;
      for(uint8_t i=0; i < oby; i++)
        DH2(obuf[i]);
      if(nibble)
        DH(obuf[oby]&0xf,1);
      if(TX_REPORT & REP_RSSI)
        DH2(cc1100_readReg(CC1100_RSSI));
#if defined(HAS_TCM97001)
        if (b->state == STATE_TCM97001) {
		  DC(';');
		  DU(b->syncbit.hightime  *16, 5);
		  DC(':');
		  DU(b->syncbit.lowtime  *16, 5);
	    }
#endif
      DNL();
      
    }

  }

#ifndef NO_RF_DEBUG
  if(TX_REPORT & REP_BITS) {
    MULTICC_PREFIX();
    DC('p');
    DU(b->state,        2);
    DU(b->zero.hightime*16, 5);
    DU(b->zero.lowtime *16, 5);
    DU(b->one.hightime *16, 5);
    DU(b->one.lowtime  *16, 5);
    DU(b->two.hightime *16, 5);
    DU(b->two.lowtime  *16, 5);
    DU(b->valCount,     4);
    DU(b->sync,         3);
    DU(b->byteidx,      3);
    DU(7-b->bitidx,     2);
#if defined(HAS_IT) || defined(HAS_TCM97001) 
    DC(' ');
    DU(b->syncbit.hightime *16, 5);
    DC(' ');
    DU(b->syncbit.lowtime  *16, 5);
    DC(' ');
#endif
    DU(b->clockTime  *16, 5);
    DC(' ');
    if(TX_REPORT & REP_RSSI) {
      DH2(cc1100_readReg(CC1100_RSSI));
      DC(' ');
    }
    if(b->bitidx != 7)
      b->byteidx++;

    for(uint8_t i=0; i < b->byteidx; i++)
       DH2(b->data[i]);
    DNL();

  }
#endif

  b->state = STATE_RESET;
  b->valCount = 0;
  maxLevel[CC_INSTANCE]=0;
  
  bucket_nrused[CC_INSTANCE]--;
  bucket_out[CC_INSTANCE]++;
  if(bucket_out[CC_INSTANCE] == RCV_BUCKETS)
    bucket_out[CC_INSTANCE] = 0;

  LED_OFF();

#ifdef HAS_FHT_80b
  if(IS868MHZ && datatype == TYPE_FHT) {
    fht_hook(obuf);
  }
#endif
}

void reset_input(void)
{
  maxLevel[CC_INSTANCE]=0;
#ifdef USE_HAL
  hal_enable_CC_timer_int(CC_INSTANCE,FALSE);
#else
  TIMSK1 = 0;
#endif
  bucket_array[CC_INSTANCE][bucket_in[CC_INSTANCE]].state = STATE_RESET;
#if defined (HAS_IT) || defined (HAS_TCM97001)
  packetCheckValues.isnotrep = 0;
#endif
#ifdef SAM7
        HAL_timer_set_reload_register(CC_INSTANCE,0);
#elif defined STM32
      	HAL_timer_set_reload_register(CC_INSTANCE,0xffff);
#else
        OCR1A = 0;
#endif
}

//////////////////////////////////////////////////////////////////////
// Timer Compare Interrupt Handler. If we are called, then there was no
// data for SILENCE time, and we can put the data to be analysed
#ifdef USE_HAL
void rf_receive_TimerElapsedCallback() {
#else
ISR(TIMER1_COMPA_vect)
{
#ifdef LONG_PULSE
  uint16_t tmp;
#endif
#endif

#ifdef SAM7
  hal_enable_CC_timer_int(CC_INSTANCE,FALSE);       //Disable Interrupt

  #ifdef LONG_PULSE
  HAL_timer_set_reload_register(CC_INSTANCE,TWRAP/8*3);    // Wrap Timer
  #endif

#elif defined STM32
  hal_enable_CC_timer_int(CC_INSTANCE,FALSE);       //Disable Interrupt

  #ifdef LONG_PULSE
  uint32_t tmp = HAL_timer_get_reload_register(CC_INSTANCE);
  HAL_timer_set_reload_register(CC_INSTANCE,TWRAP);
  HAL_timer_set_counter_value(CC_INSTANCE,tmp);
  #endif

#else
  TIMSK1 = 0;                           // Disable "us"
#ifdef LONG_PULSE
  tmp=OCR1A;
  OCR1A = TWRAP;                        // Wrap Timer
  TCNT1=tmp;                            // reinitialize timer to measure times > SILENCE
#endif
#endif
#ifndef NO_RF_DEBUG
  if(TX_REPORT & REP_MONITOR)
    DC('.');
#endif

  if(bucket_array[CC_INSTANCE][bucket_in[CC_INSTANCE]].state < STATE_COLLECT ||
     bucket_array[CC_INSTANCE][bucket_in[CC_INSTANCE]].byteidx < 2) {    // false alarm
    reset_input();
    return;

  }

  if(bucket_nrused[CC_INSTANCE]+1 == RCV_BUCKETS) {   // each bucket is full: reuse the last

#ifndef NO_RF_DEBUG
    if(TX_REPORT & REP_BITS)
      DS_P(PSTR("BOVF\r\n"));            // Bucket overflow
#endif

    reset_input();

  } else {

    bucket_nrused[CC_INSTANCE]++;
    bucket_in[CC_INSTANCE]++;
    if(bucket_in[CC_INSTANCE] == RCV_BUCKETS)
      bucket_in[CC_INSTANCE] = 0;

  }

}

uint8_t wave_equals(wave_t *a, uint8_t htime, uint8_t ltime, uint8_t state)
{
  uint8_t tdiffVal = TDIFF;
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

/*
void addBresserBit(bucket_t *b, uint8_t hightime, uint8_t lowtime) {
   if (lowtime < TSCALE(5500)) {
      if ((hightime < lowtime)) {
      //if ((lowtime < hightime)) {
        addbit(b,0);
        //DC('0');
        //b->zero.hightime = makeavg(b->zero.hightime, hightime);
        //b->zero.lowtime  = makeavg(b->zero.lowtime,  lowtime);
      } else {
        addbit(b,1);
       // DC('1');
        //b->one.hightime = makeavg(b->one.hightime, hightime);
        //b->one.lowtime  = makeavg(b->one.lowtime,  lowtime);
      }
  }
}
*/

static void
delbit(bucket_t *b)
{
  if(b->bitidx++ == 7) {           // prev byte
    b->bitidx = 0;
    b->byteidx--;
  }
}

#if defined(HAS_IT) || defined(HAS_TCM97001) 
/*
 * Calculate the end off message time
 */
static void calcOcrValue(bucket_t *b, pulse_t *hightime, pulse_t *lowtime, bool syncHighTime) {
  if (b->valCount < 8) {
    if (maxLevel[CC_INSTANCE]<*hightime) {
      maxLevel[CC_INSTANCE] = *hightime;
    } 
    if (maxLevel[CC_INSTANCE]<*lowtime) {
      maxLevel[CC_INSTANCE] = *lowtime;
    }
    if (b->valCount == 7 && maxLevel[CC_INSTANCE] != 0) {
#ifdef SAM7
        uint32_t ocrVal = 0;
#else
        uint16_t ocrVal = 0;
#endif
        if (syncHighTime) {    
          ocrVal = (((b->syncbit.hightime - maxLevel[CC_INSTANCE])>>2)+maxLevel[CC_INSTANCE]);
        } else {
          ocrVal = (((b->syncbit.lowtime - maxLevel[CC_INSTANCE])>>2)+maxLevel[CC_INSTANCE]);
        }    
#ifdef SAM7
        ocrVal = ((ocrVal * 100) / 266);
        HAL_timer_set_reload_register(CC_INSTANCE,ocrVal * 16);
#elif defined STM32
        HAL_timer_set_reload_register(CC_INSTANCE,ocrVal * 16);
#else
        OCR1A = ocrVal * 16;
#endif

    }   
  }
}
#endif

#ifdef HAS_MANCHESTER
  static uint16_t count_half;
#endif

//////////////////////////////////////////////////////////////////////
// "Edge-Detected" Interrupt Handler
#ifdef USE_HAL
	void CC1100_in_callback() {
#else
ISR(CC1100_INTVECT)
{
#endif

#ifdef HAS_FASTRF
#ifdef USE_RF_MODE
  if(is_RF_mode(RF_mode_fast)) {
    fastrf_on = 2;
    return;
  }
#else
  if(fastrf_on) {
    fastrf_on = 2;
    return;
  }
#endif
#endif
#ifdef HAS_RF_ROUTER
  if(rf_router_status == RF_ROUTER_DATA_WAIT) {
    rf_router_status = RF_ROUTER_GOT_DATA;
    return;
  }
#endif
#ifdef LONG_PULSE
  #ifdef SAM7
  uint16_t c = HAL_timer_get_counter_value(CC_INSTANCE) / 6;   // catch the time and make it smaller
  #elif defined STM32
  uint16_t c = HAL_timer_get_counter_value(CC_INSTANCE)>>4;;
  #else
  uint16_t c = (TCNT1>>4);               // catch the time and make it smaller
  #endif
#else
  #ifdef SAM7
  uint8_t c = HAL_timer_get_counter_value(CC_INSTANCE) / 6;   // catch the time and make it smaller
  #elif defined STM32
  uint8_t c = HAL_timer_get_counter_value(CC_INSTANCE)>>4;
  #else
  uint8_t c = (TCNT1>>4);               // catch the time and make it smaller
  #endif
#endif


//  if(c)
//    LED_TOGGLE();

  bucket_t *b = bucket_array[CC_INSTANCE]+bucket_in[CC_INSTANCE]; // where to fill in the bit

#ifdef HAS_HMS
  if (IS868MHZ && b->state == STATE_HMS) {
    if(c < TSCALE(750))
      return;
    if(c > TSCALE(1250)) {
      reset_input();
      return;
    }
  }
#endif

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
#ifdef USE_HAL
  if (!hal_CC_Pin_Get(CC_INSTANCE,CC_Pin_In)) {
#else
  if(!bit_is_set(CC1100_IN_PORT,CC1100_IN_PIN)) {
#endif

#if defined (HAS_HMS) || defined (HAS_ESA)
    if( IS868MHZ &&  (
#ifdef HAS_HMS
      (b->state == STATE_HMS)
#endif
#ifdef HAS_ESA
#ifdef HAS_HMS
     || 
#endif
     (IS868MHZ && b->state == STATE_ESA) 
#endif
    )) {
      addbit(b, 1);
#ifdef USE_HAL
      HAL_timer_reset_counter_value(CC_INSTANCE);
#else
      TCNT1 = 0;
#endif
    }
#endif
    
    hightime[CC_INSTANCE] = c;

#ifdef HAS_MANCHESTER
    if (b->state == STATE_MC) {
        if (hightime[CC_INSTANCE] < (b->clockTime >> 1) || hightime[CC_INSTANCE] > (b->clockTime << 1) + b->clockTime) { // read as: duration < 0.5 * clockTime || duration > 3 * clockTime
	  // Fail. Abort.
	  b->state = STATE_SYNC;
	  //DC('f');
	  return;
	}
        if (hightime[CC_INSTANCE]+(b->clockTime) > ((b->zero.hightime+b->one.hightime)>>1)) {
            b->one.hightime = makeavg(b->one.hightime,hightime[CC_INSTANCE]);
            if (b->clockTime>hightime[CC_INSTANCE]-10 && b->clockTime<hightime[CC_INSTANCE]+10) {
                count_half+=1;
            } else {
                count_half+=2;
            }
        } else {
          b->zero.hightime = makeavg(b->zero.hightime,hightime[CC_INSTANCE]);
          count_half+=1;
        }
        if (count_half&1) { // ungerade
            addbit(b,1);
        }
        
    }
#endif

    return;
  }

  lowtime[CC_INSTANCE] = c-hightime[CC_INSTANCE];

#ifdef USE_HAL
  HAL_timer_reset_counter_value(CC_INSTANCE);
#else
  TCNT1 = 0;                          // restart timer
#endif


#if defined (HAS_HMS) || defined (HAS_ESA)
  if( IS868MHZ && (
#ifdef HAS_HMS
      (b->state == STATE_HMS)
#endif
#ifdef HAS_ESA
#ifdef HAS_HMS
     || 
#endif
     (b->state == STATE_ESA) 
#endif
  )) {
    addbit(b, 0);
    return;
  }
#endif


/*  if (b->state == STATE_BRESSER && hightime>TSCALE(650) ) {//|| lowtime>TSCALE(700))
        
        DC('S');
        DNL();
        return;
  }*/



  ///////////////////////
  // http://www.nongnu.org/avr-libc/user-manual/FAQ.html#faq_intbits
#ifndef USE_HAL
  TIFR1 = _BV(OCF1A);                 // clear Timers flags (?, important!)
#endif

#ifdef HAS_MANCHESTER
    if (b->state == STATE_MC) {
retry_mc:
        // Edge is not too long, nor too short?
		if (lowtime[CC_INSTANCE] < (b->clockTime >> 1) || lowtime[CC_INSTANCE] > (b->clockTime << 1) + b->clockTime) { // read as: duration < 0.5 * clockTime || duration > 3 * clockTime
        //if (lowtime < (b->clockTime >> 1) || lowtime > (b->clockTime << 2)) {
			// Fail. Abort.
			//DC('#');
			b->state = STATE_SYNC;
		} else {
		    // Only process every second half bit, i.e. every whole bit.
		    if (lowtime[CC_INSTANCE]+(b->clockTime) > ((b->zero.lowtime+b->one.lowtime)>>1)) {
                b->one.lowtime = makeavg(b->one.lowtime,lowtime[CC_INSTANCE]);
                if (b->clockTime>lowtime[CC_INSTANCE]-10 && b->clockTime<lowtime[CC_INSTANCE]+10) {
                    count_half+=1;
                } else {
                    count_half+=2;
                }
            } else {
                b->zero.lowtime = makeavg(b->zero.lowtime,lowtime[CC_INSTANCE]);
                count_half+=1;
            }
            if (count_half&1) { // ungerade
                  addbit(b,0);
            }
            return;
	   }
    
    }
#endif

  switch(b->state)
  {
    case STATE_RESET: // first sync bit, cannot compare yet   
retry_sync:
    b->zero.hightime=0;
    b->zero.lowtime=0;
    b->one.hightime=0;
    b->one.lowtime=0;
    b->two.hightime=0;
    b->two.lowtime=0;
#ifdef HAS_MANCHESTER
    count_half = 0;
#endif
    b->clockTime = 0;
    b->valCount = 0;
    b->byteidx=0;
    b->bitidx  = 7;
    b->data[0] = 0;
#ifdef HAS_REVOLT
      if (is_revolt(b, &hightime[CC_INSTANCE], &lowtime[CC_INSTANCE])) {
        b->syncbit.hightime=hightime[CC_INSTANCE];
        b->syncbit.lowtime=lowtime[CC_INSTANCE];
        return;
      } 
#endif    
#if defined(HAS_IT) || defined(HAS_TCM97001) 
    b->syncbit.hightime=hightime[CC_INSTANCE];
    b->syncbit.lowtime=lowtime[CC_INSTANCE];
    if (IS433MHZ && (hightime[CC_INSTANCE] < TSCALE(840) && hightime[CC_INSTANCE] > TSCALE(145)) &&
				         (lowtime[CC_INSTANCE]  < TSCALE(14000) && lowtime[CC_INSTANCE] > TSCALE(3200)) ) {
	   	  // sync bit received
          b->state = STATE_SYNC_PACKAGE;
          b->sync  = 1;
	    #ifdef SAM7
          uint32_t ocrVal = 0;
          ocrVal = ((lowtime[CC_INSTANCE] * 100) / 266);
          HAL_timer_set_reload_register(CC_INSTANCE,(ocrVal - 16) * 16);
      #elif defined STM32
          HAL_timer_set_reload_register(CC_INSTANCE,(lowtime[CC_INSTANCE] - 16) * 16); //End of message
      #else
          OCR1A = (lowtime[CC_INSTANCE] - 16) * 16; //End of message
         	//OCR1A = 2200; // end of message
          //OCR1A = b->syncbit.lowtime*16 - 1000;
      #endif
      #ifdef USE_HAL
          hal_enable_CC_timer_int(CC_INSTANCE,TRUE);
      #else
          TIMSK1 = _BV(OCIE1A);
      #endif
      return;
    }
#endif
        if(hightime[CC_INSTANCE] > TSCALE(1600) || lowtime[CC_INSTANCE] > TSCALE(1600)) {
             // Invalid Package
            return;
        }
     /*   DNL();
        DC('R');
     */
      b->zero.hightime = hightime[CC_INSTANCE];
      b->zero.lowtime = lowtime[CC_INSTANCE];
      b->sync  = 1;
      b->state = STATE_SYNC;
#ifdef HAS_MANCHESTER
      if (IS433MHZ && ((hightime[CC_INSTANCE]>lowtime[CC_INSTANCE]-10 && hightime[CC_INSTANCE]<lowtime[CC_INSTANCE]+10)
         || (hightime[CC_INSTANCE]>lowtime[CC_INSTANCE]*2-10 && hightime[CC_INSTANCE]<lowtime[CC_INSTANCE]*2+10)
         || (hightime[CC_INSTANCE]>lowtime[CC_INSTANCE]/2-10 && hightime[CC_INSTANCE]<lowtime[CC_INSTANCE]/2+10))) {
        b->state = STATE_MC;
        
        // Automatic clock detection. One clock-period is half the duration of the first edge.
        b->clockTime = (hightime[CC_INSTANCE] >> 1);
        count_half=1;
        if (b->clockTime < TSCALE(300)) {
            // If clock time is lower than 300, than it should be an Oregon3
            b->clockTime = hightime[CC_INSTANCE];
        }

        // Some sanity checking, very short (<200us) or very long (>1000us) signals are ignored.
        if (b->clockTime < TSCALE(200) || b->clockTime > TSCALE(1000)) {
            b->state = STATE_SYNC;
            //DC('s');
        }
        #ifdef SAM7
        HAL_timer_set_reload_register(CC_INSTANCE,SILENCE/8*3);
        #elif defined STM32
            HAL_timer_set_reload_register(CC_INSTANCE,SILENCE);
        #else
            OCR1A = SILENCE;
        #endif
        #ifdef USE_HAL
            hal_enable_CC_timer_int(CC_INSTANCE,TRUE);
        #else
            TIMSK1 = _BV(OCIE1A);
        #endif
        b->one.lowtime = lowtime[CC_INSTANCE];
        b->one.hightime = hightime[CC_INSTANCE];
        addbit(b,1);
        
        goto retry_mc;
	}
#endif
      break;
    case STATE_SYNC:  // sync: lots of zeroes
      if(wave_equals(&b->zero, hightime[CC_INSTANCE], lowtime[CC_INSTANCE], b->state)) {
        b->zero.hightime = makeavg(b->zero.hightime, hightime[CC_INSTANCE]);
        b->zero.lowtime  = makeavg(b->zero.lowtime,  lowtime[CC_INSTANCE]);
        b->clockTime = 0;
        b->sync++;
      } else if(b->sync >= 4 ) {          // the one bit at the end of the 0-sync
#ifdef SAM7
        HAL_timer_set_reload_register(CC_INSTANCE,SILENCE/8*3);
#elif defined STM32
      	HAL_timer_set_reload_register(CC_INSTANCE,SILENCE);
#else
        OCR1A = SILENCE;
#endif
#ifdef HAS_HMS
        if (b->sync >= 12 && (b->zero.hightime + b->zero.lowtime) > TSCALE(1600)) {
          b->state = STATE_HMS;

      } else 
#endif
  #ifdef HAS_ESA
              if (b->sync >= 10 && (b->zero.hightime + b->zero.lowtime) < TSCALE(600)) {
          b->state = STATE_ESA;
          //DU(b->sync,         3);
#ifdef SAM7
          HAL_timer_set_reload_register(CC_INSTANCE,375);
#elif defined STM32
          HAL_timer_set_reload_register(CC_INSTANCE,1000);
#else
          OCR1A = 1000;
#endif

        } else 
  #endif

        
  #ifdef HAS_RF_ROUTER
          if(rf_router_myid &&
                  check_rf_sync(hightime[CC_INSTANCE], lowtime[CC_INSTANCE]) &&
                  check_rf_sync(b->zero.lowtime, b->zero.hightime)) {
          rf_router_status = RF_ROUTER_SYNC_RCVD;
          reset_input();
          return;


        } else
  #endif
        {
          b->state = STATE_COLLECT;
        }

        b->one.hightime = hightime[CC_INSTANCE];
        b->one.lowtime  = lowtime[CC_INSTANCE];
        b->byteidx = 0;
        b->bitidx  = 7;
        b->data[0] = 0;

#ifdef USE_HAL
        hal_enable_CC_timer_int(CC_INSTANCE,TRUE);
#else
        TIMSK1 = _BV(OCIE1A);             // On timeout analyze the data
#endif

      } else {                          // too few sync bits
    	  //DC('j');
    	  b->state = STATE_RESET;
        goto retry_sync;

      }
      break;
    /*case STATE_BRESSER:
    
      addBresserBit(b, hightime, lowtime);
     
      break;*/
#if defined(HAS_IT) || defined(HAS_TCM97001) 
    case STATE_SYNC_PACKAGE:
      if (lowtime[CC_INSTANCE] < TSCALE(5500)) {
#ifdef HAS_MANCHESTER      
    	if (hightime[CC_INSTANCE]>lowtime[CC_INSTANCE]-10 && hightime[CC_INSTANCE]<lowtime[CC_INSTANCE]+10 && b->valCount < 1) {
		  // Wrong message, seams to be a Oregon Sync package
		  b->state = STATE_RESET;
		  goto retry_sync;
		}
#endif

        calcOcrValue(b, &hightime[CC_INSTANCE], &lowtime[CC_INSTANCE], false);
        
        if (b->valCount == 0) {
          b->zero.hightime=hightime[CC_INSTANCE];
          b->zero.lowtime=lowtime[CC_INSTANCE];
        } 
        if (lowtime[CC_INSTANCE] < TSCALE(1800) && b->syncbit.lowtime > TSCALE(4500)) {
            // IT
            if (hightime[CC_INSTANCE] + TDIFF > lowtime[CC_INSTANCE]) {
                addbit(b, 0);
                b->one.hightime=hightime[CC_INSTANCE];
                b->one.lowtime=lowtime[CC_INSTANCE];
            } else {
                addbit(b, 1);
                b->two.hightime=hightime[CC_INSTANCE];
                b->two.lowtime=lowtime[CC_INSTANCE];
            }
        } else {
          // TCM
          if (lowtime[CC_INSTANCE] < TSCALE(2500) && b->syncbit.lowtime > TSCALE(5000)) {
              addbit(b, 0);
              b->two.hightime=hightime[CC_INSTANCE];
              b->two.lowtime=lowtime[CC_INSTANCE];
          } else if (lowtime[CC_INSTANCE] < TSCALE(1500) && b->syncbit.lowtime < TSCALE(5000)) {
              addbit(b, 0);
              b->two.hightime=hightime[CC_INSTANCE];
              b->two.lowtime=lowtime[CC_INSTANCE];
          } else {
              addbit(b, 1);
              b->one.hightime=hightime[CC_INSTANCE];
              b->one.lowtime=lowtime[CC_INSTANCE];
          }
        }
      }
      break;
#endif
#ifdef HAS_REVOLT
    case STATE_REVOLT: //STATE_REVOLTT
      //calcOcrValue(b, &hightime[CC_INSTANCE], &lowtime[CC_INSTANCE], true);
      addbit_revolt(b, &hightime[CC_INSTANCE], &lowtime[CC_INSTANCE]);
      break;
#endif

    default:
      /* TODO: Receive other sync packages
      if (b->state == STATE_COLLECT) {
        DU(hightime[CC_INSTANCE]*16, 5);
        DC(':');
        DU(lowtime[CC_INSTANCE] *16, 5);
        DC(',');
      }*/
      if(wave_equals(&b->one, hightime[CC_INSTANCE], lowtime[CC_INSTANCE], b->state)) {
        addbit(b, 1);
        b->one.hightime = makeavg(b->one.hightime, hightime[CC_INSTANCE]);
        b->one.lowtime  = makeavg(b->one.lowtime,  lowtime[CC_INSTANCE]);
      } else if(wave_equals(&b->zero, hightime[CC_INSTANCE], lowtime[CC_INSTANCE], b->state)) {
      //} else {
        addbit(b, 0);
        b->zero.hightime = makeavg(b->zero.hightime, hightime[CC_INSTANCE]);
        b->zero.lowtime  = makeavg(b->zero.lowtime,  lowtime[CC_INSTANCE]);
     /* TODO: Receive other sync packages
      } else if(hightime[CC_INSTANCE] > lowtime[CC_INSTANCE]) {
        addbit(b, 1);*/
      } else {
      /*  addbit(b, 0);*/
      /*  b->two.hightime = makeavg(b->zero.hightime, hightime[CC_INSTANCE]);
        b->two.lowtime  = makeavg(b->zero.lowtime,  lowtime[CC_INSTANCE]);*/
        reset_input();
      }
      break;
  }

}

uint8_t
rf_isreceiving()
{
  uint8_t r = (bucket_array[CC_INSTANCE][bucket_in[CC_INSTANCE]].state != STATE_RESET);
#ifdef HAS_FHT_80b
  r = (r || fht80b_timeout != FHT_TIMER_DISABLED);
#endif
  return r;
}

