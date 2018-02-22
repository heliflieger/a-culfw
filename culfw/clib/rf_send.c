/* 
 * Copyright by R.Koenig
 * Inspired by code from Dirk Tostmann
 * License: GPL v2
 */

#include <avr/interrupt.h>              // for cli, sei
#include <avr/pgmspace.h>               // for PSTR
#include <avr/io.h>                     // for _BV
#include <stdint.h>                     // for int8_t
#include <util/parity.h>                // for parity_even_bit

#include "board.h"                      // for HAS_MORITZ, HAS_RAWSEND, etc
#include "cc1100.h"                     // for CC1100_CLEAR_OUT, etc
#include "delay.h"                      // for my_delay_us, my_delay_ms
#include "display.h"                    // for DS_P
#include "led.h"                        // for LED_OFF, LED_ON
#include "rf_receive.h"                 // for cksum1, cksum2, cksum3, etc
#include "rf_send.h"
#include "stringfunc.h"                 // for fromhex
#include "rf_mode.h"
#include "i2cslave.h"										// for TWCR_INIT, TWCR_OFF

#ifdef HAS_DMX
#include "dmx.h"                        // for dmx_fs20_emu
#endif

#ifdef HAS_HELIOS
#include "helios.h"                     // for helios_fs20_emu
#endif

#ifdef HAS_MORITZ
#include "rf_moritz.h"                  // for moritz_on, rf_moritz_init
#endif

// For FS20 we time the complete message, for KS300 the rise-fall distance
// FS20  NULL: 400us high, 400us low
// FS20  ONE:  600us high, 600us low
// KS300 NULL  854us high, 366us low
// KS300 ONE:  366us high, 854us low

#define FS20_ZERO      400     //   400uS
#define FS20_ONE       600     //   600uS
#define FS20_PAUSE      10     // 10000mS
#define EM_ONE         800     //   800uS
#define EM_ZERO        400     //   400uS

#if defined(HAS_HOERMANN_SEND)
#define HRM_ZERO_H         992 //us
#define HRM_ZERO_L         448 //us
#define HRM_ONE_H          528 //us
#define HRM_ONE_L          928 //us
#define HRM_EXTRA_SYNC_H  4000 //us  measured with CUL raw read output
#define HRM_EXTRA_SYNC_L   600 //us  measured with CUL raw read output
#endif

uint16_t credit_10ms;

#define TMUL(x) (x<<4)
#define TDIV(x) (x>>4)


#if defined(HAS_RAWSEND) || defined(HAS_HOERMANN_SEND)

#  define MAX_SNDMSG 12
#  define MAX_SNDRAW 12

  static uint8_t zerohigh, zerolow, onehigh, onelow;

  static void
  send_bit(uint8_t bit)
  {
    CC1100_SET_OUT;         // High
    my_delay_us(bit ? TMUL(onehigh) : TMUL(zerohigh));

    CC1100_CLEAR_OUT;       // Low
    my_delay_us(bit ? TMUL(onelow) : TMUL(zerolow));
  }

#else

#  define MAX_SNDMSG 6    // FS20: 4 or 5 + CRC, FHT: 5+CRC
#  define MAX_SNDRAW 7    // MAX_SNDMSG*9/8 (parity bit)

  static void
  send_bit(uint8_t bit)
  {
    CC1100_SET_OUT;         // High
    my_delay_us(bit ? FS20_ONE : FS20_ZERO);

    CC1100_CLEAR_OUT;       // Low
    my_delay_us(bit ? FS20_ONE : FS20_ZERO);
  }

#endif

// msg is with parity/checksum already added
static void
sendraw(uint8_t *msg, uint8_t sync, uint8_t nbyte, uint8_t bitoff,
                uint8_t repeat, uint8_t pause, uint8_t addH, uint8_t addL)
{
  // 12*800+1200+nbyte*(8*1000)+(bits*1000)+800+10000 
  // message len is < (nbyte+2)*repeat in 10ms units.
  int8_t i, j, sum = (nbyte+2)*repeat + addH + addL;
  if (credit_10ms < sum) {
    DS_P(PSTR("LOVF\r\n"));
    return;
  }
  credit_10ms -= sum;

  LED_ON();

#if defined (HAS_IRRX) || defined (HAS_IRTX) || defined (HAS_I2CSLAVE) // Block IR_Reception
  cli();
#  if defined (HAS_I2CSLAVE) // Disable I2C
    TWCR_OFF;
#  endif		
#endif

#ifdef USE_RF_MODE
  change_RF_mode(RF_mode_slow);
#else
#ifdef HAS_MORITZ
  uint8_t restore_moritz = 0;
  if(moritz_on) {
    restore_moritz = 1;
    moritz_on = 0;
    set_txreport("21");
  }
#endif
  if(!cc_on)
    set_ccon();
#endif
  ccTX();                                       // Enable TX 
  do {

    if(addH>0 || addL>0) {
      CC1100_SET_OUT;         // High
      my_delay_us(TMUL(addH));

      CC1100_CLEAR_OUT;       // Low
      my_delay_us(TMUL(addL));
    }

    for(i = 0; i < sync; i++)                   // sync
      send_bit(0);
    if(sync)
      send_bit(1);
    
    for(j = 0; j < nbyte; j++) {                // whole bytes
      for(i = 7; i >= 0; i--)
        send_bit(msg[j] & _BV(i));
    }
    for(i = 7; i > bitoff; i--)                 // broken bytes
      send_bit(msg[j] & _BV(i));

    my_delay_ms(pause);                         // pause

  } while(--repeat > 0);

  if(TX_REPORT) {                               // Enable RX
    ccRX();
  } else {
    ccStrobe(CC1100_SIDLE);
  }

#if defined (HAS_IRRX) || defined (HAS_IRTX) || defined(HAS_I2CSLAVE) // Activate IR_Reception
  sei(); 
#  if defined (HAS_I2CSLAVE) // Enable TWI again
    TWCR_INIT;
#  endif				
#endif

#ifdef USE_RF_MODE
  restore_RF_mode();
#else
#ifdef HAS_MORITZ
  if(restore_moritz)
    rf_moritz_init();
#endif
#endif

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
    if(oby < MAX_SNDRAW)
      *obyp = oby;
    obi = 7; obuf[oby] = 0;
  }
  return obi;
}

void
addParityAndSendData(uint8_t *hb, uint8_t hblen,
                uint8_t startcs, uint8_t repeat)
{
  uint8_t iby, obuf[MAX_SNDRAW], oby;
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

#if defined(HAS_RAWSEND) || defined(HAS_HOERMANN_SEND)
  zerohigh = zerolow = TDIV(FS20_ZERO);
  onehigh = onelow = TDIV(FS20_ONE);
#endif
  sendraw(obuf, 12, oby, obi, repeat, FS20_PAUSE, 0, 0);
}

void
addParityAndSend(char *in, uint8_t startcs, uint8_t repeat)
{
  uint8_t hb[MAX_SNDMSG], hblen;
  hblen = fromhex(in+1, hb, MAX_SNDMSG-1);
  addParityAndSendData(hb, hblen, startcs, repeat);
}


void
fs20send(char *in)
{
#ifdef HAS_DMX
  if(dmx_fs20_emu( in ))
    return;
#endif
#ifdef HAS_HELIOS
  if (helios_fs20_emu( in ))
    return;
#endif
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
  sendraw(hb+7, sync, nby, nbi, repeat, pause, 0, 0);
}


// E0205E7000000000000
void
em_send(char *in)
{
  uint8_t iby, obuf[MAX_SNDRAW], oby;
  int8_t  ibi, obi;
  uint8_t hb[MAX_SNDMSG];

  uint8_t hblen = fromhex(in+1, hb, MAX_SNDMSG-1);

  // EM is always 9 bytes payload!
  if (hblen != 9) {
//  DS_P(PSTR("LENERR\r\n"));
    return;
  }
  
  onehigh = zerohigh = zerolow = TDIV(EM_ZERO);
  onelow  = TDIV(EM_ONE);

  // calc checksum
  hb[hblen] = cksum2( hb, hblen );
  hblen++;

  // Copy the message and add parity-bits
  iby=oby=0;
  ibi=obi=7;
  obuf[oby] = 0;
  
  while(iby<hblen) {
    obi = abit(hb[iby] & _BV(7-ibi), obuf, &oby, obi);
    if(ibi-- == 0) {
      obi = abit(1, obuf, &oby, obi); // always 1
      ibi = 7; iby++;
    }
  }
  if(obi-- == 0) {                   // Trailing 0 bit: indicating EOM
    oby++; obi = 7;
  }

  sendraw(obuf, 12, oby, obi, 3, FS20_PAUSE, 0, 0);
}

void
ks_send(char *in)
{
  uint8_t iby, obuf[MAX_SNDRAW], oby;
  int8_t  ibi, obi;
  uint8_t hb[MAX_SNDMSG];

  uint8_t hblen = fromhex(in+1, hb, MAX_SNDMSG-1);

//  KS may have different length - TODO: byte padding for some sensor types
//  if (hblen != 9) {
//  DS_P(PSTR("KS send\r\n"));
//    return;
//  }

  zerohigh = TDIV(855); 
  zerolow  = TDIV(366);
  onehigh  = TDIV(366); 
  onelow   = TDIV(855);

  // calc checksum
  hb[hblen] = cksum3( hb, hblen );
  hblen++;

  // Copy the message and add parity-bits
  iby=oby=0;
  ibi=obi=7;
  obuf[oby] = 0;

  while(iby<hblen) {

    for (ibi=0; ibi<4; ibi++)
      obi = abit(hb[iby] & _BV(ibi), obuf, &oby, obi);

    obi = abit(1, obuf, &oby, obi); // always 1

    for (ibi=4; ibi<8; ibi++)
      obi = abit(hb[iby] & _BV(ibi), obuf, &oby, obi);

    obi = abit(1, obuf, &oby, obi); // always 1

    iby++;
  }

  sendraw(obuf, 10, oby, obi, 3, FS20_PAUSE, 0, 0);
}

#endif

#ifdef HAS_UNIROLL
//G0031E368232368hhhhdc1
//16 bit group-address 4 bit channel-address 4 bit command 1 1-bit

void
ur_send(char *in)
{
  uint8_t hb[4];

  fromhex(in+1, hb, 3);
  zerohigh = TDIV(1700);
  zerolow  = TDIV(590);
  onehigh  = TDIV(540);
  onelow   = TDIV(1700);
  hb[3] = 0x80;     //10000000
  sendraw(hb, 0, 3, 6, 3, 15, 0, 0);
}
#endif

#ifdef HAS_HOERMANN_SEND
void
hm_send(char *in)
{
  uint8_t hb[MAX_SNDMSG];
  uint8_t hblen = fromhex(in + 2, hb, MAX_SNDMSG - 1);

  if(hblen != 5)       // LENERR
    return;
  zerohigh = TDIV(HRM_ZERO_H);
  zerolow = TDIV(HRM_ZERO_L);
  onehigh = TDIV(HRM_ONE_H);
  onelow = TDIV(HRM_ONE_L);

  sendraw(hb, 8, 4, 4, 5, 0, TDIV(HRM_EXTRA_SYNC_H), TDIV(HRM_EXTRA_SYNC_L));
}
#endif
