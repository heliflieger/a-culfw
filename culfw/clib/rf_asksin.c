#include "board.h"
#ifdef HAS_ASKSIN
#include <string.h>
#include <avr/pgmspace.h>
#include "cc1100.h"
#include "delay.h"
#include "rf_receive.h"
#include "display.h"

#include "rf_asksin.h"

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

static void rf_asksin_reset_rx(void);

void
rf_asksin_init(void)
{

  EIMSK &= ~_BV(CC1100_INT);                 // disable INT - we'll poll...
  SET_BIT( CC1100_CS_DDR, CC1100_CS_PIN );   // CS as output

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
  
  ccStrobe( CC1100_SCAL );

  my_delay_ms(4);

  ccRX();
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
  uint8_t enc[MAX_ASKSIN_MSG];
  uint8_t dec[MAX_ASKSIN_MSG];
  uint8_t rssi;
  uint8_t l;

  if(!asksin_on)
    return;

  // see if a CRC OK pkt has been arrived
  if (bit_is_set( CC1100_IN_PORT, CC1100_IN_PIN )) {
    enc[0] = cc1100_readReg( CC1100_RXFIFO ) & 0x7f; // read len

    if (enc[0] >= MAX_ASKSIN_MSG) {
      // Something went horribly wrong, out of sync?
      rf_asksin_reset_rx();
      return;
    }

    CC1100_ASSERT;
    cc1100_sendbyte( CC1100_READ_BURST | CC1100_RXFIFO );
    
    for (uint8_t i=0; i<enc[0]; i++) {
         enc[i+1] = cc1100_sendbyte( 0 );
    }
    
    rssi = cc1100_sendbyte( 0 );
    /* LQI = */ cc1100_sendbyte( 0 );

    CC1100_DEASSERT;

    ccStrobe( CC1100_SRX   );

    dec[0] = enc[0];
    dec[1] = (~enc[1]) ^ 0x89;
    
    for (l = 2; l < dec[0]; l++)
         dec[l] = (enc[l-1] + 0xdc) ^ enc[l];
    
    dec[l] = enc[l] ^ dec[2];
    
    if (tx_report & REP_BINTIME) {
      
      DC('a');
      for (uint8_t i=0; i<=dec[0]; i++)
      DC( dec[i] );
         
    } else {
      DC('A');
      
      for (uint8_t i=0; i<=dec[0]; i++)
        DH2( dec[i] );
      
      if (tx_report & REP_RSSI)
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
}

void
asksin_send(char *in)
{
  uint8_t enc[MAX_ASKSIN_MSG];
  uint8_t dec[MAX_ASKSIN_MSG];
  uint8_t l;

  uint8_t hblen = fromhex(in+1, dec, MAX_ASKSIN_MSG-1);

  if ((hblen-1) != dec[0]) {
//  DS_P(PSTR("LENERR\r\n"));
    return;
  }

  // in AskSin mode already?
  if(!asksin_on) {
    rf_asksin_init();
    my_delay_ms(3);             // 3ms: Found by trial and error
  }

  // "crypt"
  enc[0] = dec[0];
  enc[1] = (~dec[1]) ^ 0x89;

  for (l = 2; l < dec[0]; l++)
    enc[l] = (enc[l-1] + 0xdc) ^ dec[l];
  
  enc[l] = dec[l] ^ dec[2];

  ccTX();
  if (dec[2] & (1 << 4)) { // BURST-bit set?
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
    cc1100_sendbyte(enc[i]);
  }

  CC1100_DEASSERT;

  // wait for TX to finish
  while(cc1100_readReg( CC1100_MARCSTATE ) == MARCSTATE_TX)
    ;

  if (cc1100_readReg( CC1100_MARCSTATE ) == MARCSTATE_TXFIFO_UNDERFLOW) {
      ccStrobe( CC1100_SFTX  );
      ccStrobe( CC1100_SIDLE );
      ccStrobe( CC1100_SNOP  );
      ccStrobe( CC1100_SRX   );
  }
  
  if(asksin_on) {
    ccRX();
  } else {
    set_txrestore();
  }
}

void
asksin_func(char *in)
{
  if(in[1] == 'r') {                // Reception on
    rf_asksin_init();
    asksin_on = 1;

  } else if(in[1] == 's') {         // Send
    asksin_send(in+1);

  } else {                          // Off
    asksin_on = 0;

  }
}

#endif
