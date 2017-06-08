#include <avr/io.h>                     // for _BV, bit_is_set
#include <stdint.h>                     // for uint8_t, uint32_t

#include "board.h"                      // for HAS_ASKSIN_FUP, etc
#include "led.h"                        // for SET_BIT
#include "stringfunc.h"                 // for fromhex
#ifdef HAS_ASKSIN
#include <avr/pgmspace.h>               // for pgm_read_byte, PROGMEM, etc

#include "cc1100.h"                     // for ccStrobe, cc1100_readReg, etc
#include "cc1101_pllcheck.h"
#include "clock.h"                      // for get_timestamp
#include "delay.h"                      // for my_delay_ms, my_delay_us
#include "display.h"                    // for DC, DH2, DNL, DS_P
#include "rf_asksin.h"
#include "rf_receive.h"                 // for set_txrestore, REP_BINTIME, etc
#include "rf_mode.h"
#include "multi_CC.h"

#ifdef USE_HAL
#include "hal.h"
#endif

#ifndef USE_RF_MODE
uint8_t asksin_on = 0;
#endif

const uint8_t PROGMEM ASKSIN_CFG[] = {
     0x00, 0x07,
     0x02, 0x2e,
     0x03, 0x0d,
     0x04, 0xE9,
     0x05, 0xCA,
     0x07, 0x0C,
     0x0B, 0x06,
     0x0D, 0x21,
     0x0E, 0x65,
     0x0F, 0x6A,
     0x10, 0xC8,
     0x11, 0x93,
     0x12, 0x03,
     0x15, 0x34,
     0x17, 0x33, // go into RX after TX, CCA; EQ3 uses 0x03
     0x18, 0x18,
     0x19, 0x16,
     0x1B, 0x43,
     0x21, 0x56,
     0x25, 0x00,
     0x26, 0x11,
     0x29, 0x59,
     0x2c, 0x81,
     0x2D, 0x35,
     0x3e, 0xc3
};

#ifdef HAS_ASKSIN_FUP
const uint8_t PROGMEM ASKSIN_UPDATE_CFG[] = {
     0x0B, 0x08,
     0x10, 0x5B,
     0x11, 0xF8,
     0x15, 0x47,
     0x19, 0x1D,
     0x1A, 0x1C,
     0x1B, 0xC7,
     0x1C, 0x00,
     0x1D, 0xB2,
     0x21, 0xB6,
     0x23, 0xEA,
};

static unsigned char asksin_update_mode = 0;
#endif

static void rf_asksin_reset_rx(void);

void
rf_asksin_init(void)
{

#ifdef USE_HAL
  hal_CC_GDO_init(CC_INSTANCE,INIT_MODE_IN_CS_IN);
  hal_enable_CC_GDOin_int(CC_INSTANCE,FALSE); // disable INT - we'll poll...
#else
  EIMSK &= ~_BV(CC1100_INT);                 // disable INT - we'll poll...
  SET_BIT( CC1100_CS_DDR, CC1100_CS_PIN );   // CS as output
#endif

  CC1100_DEASSERT;                           // Toggle chip select signal
  my_delay_us(30);
  CC1100_ASSERT;
  my_delay_us(30);
  CC1100_DEASSERT;
  my_delay_us(45);

  ccStrobe( CC1100_SRES );                   // Send SRES command
  my_delay_us(100);

  // load configuration
  for (uint8_t i = 0; i < sizeof(ASKSIN_CFG); i += 2) {
    cc1100_writeReg( pgm_read_byte(&ASKSIN_CFG[i]),
                     pgm_read_byte(&ASKSIN_CFG[i+1]) );
  }

#ifdef HAS_ASKSIN_FUP
  if (asksin_update_mode) {
    for (uint8_t i = 0; i < sizeof(ASKSIN_UPDATE_CFG); i += 2) {
      cc1100_writeReg( pgm_read_byte(&ASKSIN_UPDATE_CFG[i]),
                       pgm_read_byte(&ASKSIN_UPDATE_CFG[i+1]) );
    }
  }
#endif
  
  ccStrobe( CC1100_SCAL );

  my_delay_ms(4);

  // enable RX, but don't enable the interrupt
  do {
    ccStrobe(CC1100_SRX);
  } while (cc1100_readReg(CC1100_MARCSTATE) != MARCSTATE_RX);
}

static void
rf_asksin_reset_rx(void)
{
  ccStrobe( CC1100_SFRX  );
  ccStrobe( CC1100_SIDLE );
  ccStrobe( CC1100_SNOP  );
  ccStrobe( CC1100_SRX   );
}

void
rf_asksin_task(void)
{
  uint8_t msg[MAX_ASKSIN_MSG];
  uint8_t this_enc, last_enc;
  uint8_t rssi;
  uint8_t l;

#ifndef USE_RF_MODE
  if(!asksin_on)
    return;
#endif

  // see if a CRC OK pkt has been arrived
#ifdef USE_HAL
  if (hal_CC_Pin_Get(CC_INSTANCE,CC_Pin_In)) {
#else
  if (bit_is_set( CC1100_IN_PORT, CC1100_IN_PIN )) {
#endif
    msg[0] = cc1100_readReg( CC1100_RXFIFO ) & 0x7f; // read len

    if (msg[0] >= MAX_ASKSIN_MSG) {
      // Something went horribly wrong, out of sync?
      rf_asksin_reset_rx();
      return;
    }

    CC1100_ASSERT;
    cc1100_sendbyte( CC1100_READ_BURST | CC1100_RXFIFO );
    
    for (uint8_t i=0; i<msg[0]; i++) {
         msg[i+1] = cc1100_sendbyte( 0 );
    }
    
    rssi = cc1100_sendbyte( 0 );
    /* LQI = */ cc1100_sendbyte( 0 );

    CC1100_DEASSERT;

    do {
      ccStrobe(CC1100_SRX);
    } while (cc1100_readReg(CC1100_MARCSTATE) != MARCSTATE_RX);

    last_enc = msg[1];
    msg[1] = (~msg[1]) ^ 0x89;
    
    for (l = 2; l < msg[0]; l++) {
         this_enc = msg[l];
         msg[l] = (last_enc + 0xdc) ^ msg[l];
         last_enc = this_enc;
    }
    
    msg[l] = msg[l] ^ msg[2];

    MULTICC_PREFIX();

    if (TX_REPORT & REP_BINTIME) {

      DC('a');
      for (uint8_t i=0; i<=msg[0]; i++)
      DC( msg[i] );
         
    } else {
      DC('A');
      
      for (uint8_t i=0; i<=msg[0]; i++)
        DH2( msg[i] );
      if (TX_REPORT & REP_RSSI)
        DH2(rssi);
      
      DNL();
    }
  }

  switch(cc1100_readReg( CC1100_MARCSTATE )) {
    case MARCSTATE_RXFIFO_OVERFLOW:
      ccStrobe( CC1100_SFRX  );
    case MARCSTATE_IDLE:
      ccStrobe( CC1100_SIDLE );
      ccStrobe( CC1100_SNOP  );
      ccStrobe( CC1100_SRX   );
      break;
  }

#ifdef HAS_CC1101_RX_PLL_LOCK_CHECK_TASK_WAIT
  cc1101_RX_check_PLL_wait_task();
#endif
}

void
asksin_send(char *in)
{
  uint8_t msg[MAX_ASKSIN_MSG];
  uint8_t ctl;
  uint8_t l;
  uint32_t ts1, ts2;

  uint8_t hblen = fromhex(in+1, msg, MAX_ASKSIN_MSG-1);

  if ((hblen-1) != msg[0]) {
//  DS_P(PSTR("LENERR\r\n"));
    return;
  }

#ifdef USE_RF_MODE
  change_RF_mode(RF_mode_asksin);
#else
  // in AskSin mode already?
  if(!asksin_on) {
    rf_asksin_init();
    my_delay_ms(3);             // 3ms: Found by trial and error
  }
#endif

  ctl = msg[2];

  // "crypt"
  msg[1] = (~msg[1]) ^ 0x89;

  for (l = 2; l < msg[0]; l++)
    msg[l] = (msg[l-1] + 0xdc) ^ msg[l];
  
  msg[l] = msg[l] ^ ctl;

  // enable TX, wait for CCA
  get_timestamp(&ts1);
  do {
    ccStrobe(CC1100_STX);
    if (cc1100_readReg(CC1100_MARCSTATE) != MARCSTATE_TX) {
      get_timestamp(&ts2);
      if (((ts2 > ts1) && (ts2 - ts1 > ASKSIN_WAIT_TICKS_CCA)) ||
          ((ts2 < ts1) && (ts1 + ASKSIN_WAIT_TICKS_CCA < ts2))) {
        MULTICC_PREFIX();
        DS_P(PSTR("ERR:CCA\r\n"));
        goto out;
      }
    }
  } while (cc1100_readReg(CC1100_MARCSTATE) != MARCSTATE_TX);

  if (ctl & (1 << 4)) { // BURST-bit set?
    // According to ELV, devices get activated every 300ms, so send burst for 360ms
    for(l = 0; l < 3; l++)
      my_delay_ms(120); // arg is uint_8, so loop
  } else {
  	my_delay_ms(10);
  }

  // send
  CC1100_ASSERT;
  cc1100_sendbyte(CC1100_WRITE_BURST | CC1100_TXFIFO);

  for(uint8_t i = 0; i < hblen; i++) {
    cc1100_sendbyte(msg[i]);
  }

  CC1100_DEASSERT;

  // wait for TX to finish
  while(cc1100_readReg( CC1100_MARCSTATE ) == MARCSTATE_TX)
    ;

out:
  if (cc1100_readReg( CC1100_MARCSTATE ) == MARCSTATE_TXFIFO_UNDERFLOW) {
      ccStrobe( CC1100_SFTX  );
      ccStrobe( CC1100_SIDLE );
      ccStrobe( CC1100_SNOP  );
  }
#ifdef USE_RF_MODE
  if(!restore_RF_mode()) {
    do {
      ccStrobe(CC1100_SRX);
    } while (cc1100_readReg(CC1100_MARCSTATE) != MARCSTATE_RX);
  }
#else
  if(asksin_on) {
    do {
      ccStrobe(CC1100_SRX);
    } while (cc1100_readReg(CC1100_MARCSTATE) != MARCSTATE_RX);
  } else {
    set_txrestore();
  }
#endif
}

void
asksin_func(char *in)
{
#ifndef HAS_ASKSIN_FUP
  if(in[1] == 'r') {                // Reception on
#else
  if((in[1] == 'r') || (in[1] == 'R')) {                // Reception on
    if (in[1] == 'R') {
      asksin_update_mode = 1;
    } else {
      asksin_update_mode = 0;
    }
#endif
#ifdef USE_RF_MODE
    set_RF_mode(RF_mode_asksin);
#else
    rf_asksin_init();
    asksin_on = 1;
#endif

  } else if(in[1] == 's') {         // Send
    asksin_send(in+1);

  } else {                          // Off
#ifdef USE_RF_MODE
    set_RF_mode(RF_mode_off);
#else
    asksin_on = 0;
#endif

  }
}

#endif
