#include "board.h"
#ifdef HAS_ASKSIN
#include <string.h>
#include <avr/pgmspace.h>
#include "cc1100.h"
#include "delay.h"
#include "rf_receive.h"
#include "display.h"
#include "cc1101_pllcheck.h"
#include "clock.h"

#include "rf_asksin.h"

#ifdef CC1100_ASKSIN
#include "fncollection.h"

#define CC_ID				CC1100_ASKSIN

extern void cc1101_RX_check_PLL_wait_task2( transceiver_t* device);

#undef CC1100_ASSERT
#define CC1100_ASSERT 		(CCtransceiver[CC_ID].CS_base->PIO_CODR = (1<<CCtransceiver[CC_ID].CS_pin))
#undef CC1100_DEASSERT
#define CC1100_DEASSERT 	(CCtransceiver[CC_ID].CS_base->PIO_SODR = (1<<CCtransceiver[CC_ID].CS_pin))
#undef CC1100_IN_PORT
#define CC1100_IN_PORT 		(CCtransceiver[CC_ID].CS_base->PIO_PDSR)
#undef CC1100_IN_PIN
#define CC1100_IN_PIN 		CCtransceiver[CC_ID].IN_pin
#undef CC1100_CS_BASE
#define CC1100_CS_BASE 		CCtransceiver[CC_ID].CS_base
#undef CC1100_CS_PIN
#define CC1100_CS_PIN 		CCtransceiver[CC_ID].CS_pin


#define CC1100_READREG(x)				cc1100_readReg2(x,&CCtransceiver[CC_ID])
#define CC1100_WRITEREG(x,y) 			cc1100_writeReg2(x,y,&CCtransceiver[CC_ID])
#define CCSTROBE(x) 					ccStrobe2(x,&CCtransceiver[CC_ID])
#define CC1101_RX_CHECK_PLL_WAIT_TASK()	cc1101_RX_check_PLL_wait_task2(&CCtransceiver[CC_ID])

#else

#define CC1100_READREG					cc1100_readReg
#define CC1100_WRITEREG 				cc1100_writeReg
#define CCSTROBE						ccStrobe
#define CC1101_RX_CHECK_PLL_WAIT_TASK	cc1101_RX_check_PLL_wait_task
#endif

uint8_t asksin_on = 0;

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

#ifdef ARM
#ifndef CC_ID
  AT91C_BASE_AIC->AIC_IDCR = 1 << CC1100_IN_PIO_ID;	// disable INT - we'll poll...
#endif

  CC1100_CS_BASE->PIO_PPUER = _BV(CC1100_CS_PIN); 		//Enable pullup
  CC1100_CS_BASE->PIO_OER = _BV(CC1100_CS_PIN);			//Enable output
  CC1100_CS_BASE->PIO_PER = _BV(CC1100_CS_PIN);			//Enable PIO control

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

  CCSTROBE( CC1100_SRES );                   // Send SRES command
  my_delay_us(100);

#ifdef CC_ID
  CC1100_ASSERT;
  uint8_t *cfg = EE_CC1100_CFG;
  for(uint8_t i = 0; i < EE_CC1100_CFG_SIZE; i++) {
      cc1100_sendbyte(erb(cfg++));
  }
  CC1100_DEASSERT;

  uint8_t *pa = EE_CC1100_PA;
    CC1100_ASSERT;                             // setup PA table
    cc1100_sendbyte( CC1100_PATABLE | CC1100_WRITE_BURST );
    for (uint8_t i = 0;i<8;i++) {
      cc1100_sendbyte(erb(pa++));
    }
    CC1100_DEASSERT;
#endif
  // load configuration
  for (uint8_t i = 0; i < sizeof(ASKSIN_CFG); i += 2) {
    CC1100_WRITEREG( pgm_read_byte(&ASKSIN_CFG[i]),
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
  
  CCSTROBE( CC1100_SCAL );

  my_delay_ms(4);

  // enable RX, but don't enable the interrupt
  do {
    CCSTROBE(CC1100_SRX);
  } while (CC1100_READREG(CC1100_MARCSTATE) != MARCSTATE_RX);
}

static void
rf_asksin_reset_rx(void)
{
  CCSTROBE( CC1100_SFRX  );
  CCSTROBE( CC1100_SIDLE );
  CCSTROBE( CC1100_SNOP  );
  CCSTROBE( CC1100_SRX   );
}

void
rf_asksin_task(void)
{
  uint8_t msg[MAX_ASKSIN_MSG];
  uint8_t this_enc, last_enc;
  uint8_t rssi;
  uint8_t l;

  if(!asksin_on)
    return;

  // see if a CRC OK pkt has been arrived
  if (bit_is_set( CC1100_IN_PORT, CC1100_IN_PIN )) {
    msg[0] = CC1100_READREG( CC1100_RXFIFO ) & 0x7f; // read len

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
      CCSTROBE(CC1100_SRX);
    } while (CC1100_READREG(CC1100_MARCSTATE) != MARCSTATE_RX);

    last_enc = msg[1];
    msg[1] = (~msg[1]) ^ 0x89;
    
    for (l = 2; l < msg[0]; l++) {
         this_enc = msg[l];
         msg[l] = (last_enc + 0xdc) ^ msg[l];
         last_enc = this_enc;
    }
    
    msg[l] = msg[l] ^ msg[2];
    
    if (tx_report & REP_BINTIME) {
      
      DC('a');
      for (uint8_t i=0; i<=msg[0]; i++)
      DC( msg[i] );
         
    } else {
      DC('A');
      
      for (uint8_t i=0; i<=msg[0]; i++)
        DH2( msg[i] );
      
      if (tx_report & REP_RSSI)
        DH2(rssi);
      
      DNL();
    }
  }

  switch(CC1100_READREG( CC1100_MARCSTATE )) {
    case MARCSTATE_RXFIFO_OVERFLOW:
      CCSTROBE( CC1100_SFRX  );
    case MARCSTATE_IDLE:
      CCSTROBE( CC1100_SIDLE );
      CCSTROBE( CC1100_SNOP  );
      CCSTROBE( CC1100_SRX   );
      break;
  }

#ifdef HAS_CC1101_RX_PLL_LOCK_CHECK_TASK_WAIT
  CC1101_RX_CHECK_PLL_WAIT_TASK();
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

  // in AskSin mode already?
  if(!asksin_on) {
    rf_asksin_init();
    my_delay_ms(3);             // 3ms: Found by trial and error
  }

  ctl = msg[2];

  // "crypt"
  msg[1] = (~msg[1]) ^ 0x89;

  for (l = 2; l < msg[0]; l++)
    msg[l] = (msg[l-1] + 0xdc) ^ msg[l];
  
  msg[l] = msg[l] ^ ctl;

  // enable TX, wait for CCA
  get_timestamp(&ts1);
  do {
    CCSTROBE(CC1100_STX);
    if (CC1100_READREG(CC1100_MARCSTATE) != MARCSTATE_TX) {
      get_timestamp(&ts2);
      if (((ts2 > ts1) && (ts2 - ts1 > ASKSIN_WAIT_TICKS_CCA)) ||
          ((ts2 < ts1) && (ts1 + ASKSIN_WAIT_TICKS_CCA < ts2))) {
        DS_P(PSTR("ERR:CCA\r\n"));
        goto out;
      }
    }
  } while (CC1100_READREG(CC1100_MARCSTATE) != MARCSTATE_TX);

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
  while(CC1100_READREG( CC1100_MARCSTATE ) == MARCSTATE_TX)
    ;

out:
  if (CC1100_READREG( CC1100_MARCSTATE ) == MARCSTATE_TXFIFO_UNDERFLOW) {
      CCSTROBE( CC1100_SFTX  );
      CCSTROBE( CC1100_SIDLE );
      CCSTROBE( CC1100_SNOP  );
  }
  
  if(asksin_on) {
    do {
      CCSTROBE(CC1100_SRX);
    } while (CC1100_READREG(CC1100_MARCSTATE) != MARCSTATE_RX);
  } else {
    set_txrestore();
  }
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
    rf_asksin_init();
    asksin_on = 1;

  } else if(in[1] == 's') {         // Send
    asksin_send(in+1);

  } else {                          // Off
    asksin_on = 0;

  }
}

#endif
