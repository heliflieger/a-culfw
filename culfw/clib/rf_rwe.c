#include <avr/io.h>                     // for _BV, bit_is_set
#include <stdint.h>                     // for uint8_t

#include "board.h"                      // for CC1100_CS_DDR, etc
#include "led.h"                        // for SET_BIT
#include "stringfunc.h"                 // for fromhex
#ifdef HAS_RWE
#include <avr/pgmspace.h>               // for pgm_read_byte, PROGMEM

#include "cc1100.h"                     // for ccStrobe, CC1100_DEASSERT, etc
#include "delay.h"                      // for my_delay_ms, my_delay_us
#include "display.h"                    // for DC, DH2, DNL
#include "fband.h"                      // for checkFrequency
#include "rf_receive.h"                 // for set_txrestore, REP_BINTIME, etc
#include "rf_rwe.h"
#include "rf_mode.h"
#include "multi_CC.h"

#ifdef USE_HAL
#include "hal.h"
#endif

#ifndef USE_RF_MODE
uint8_t rwe_on = 0;
#endif

const uint8_t PROGMEM RWE_CFG[60] = {
//     0x00, 0x0E,
//     0x02, 0x06,
     0x00, 0x07, // we can relax until pkt is received CRC ok!
     0x02, 0x46, // not used
     0x03, 0x0D,
     0x04, 0x9A,
     0x05, 0x7D,
     0x06, 0x42,
     0x07, 0x0C,
     0x0B, 0x06,
     0x0D, 0x21,
     0x0E, 0x65,
     0x0F, 0x6A,
     0x10, 0xC8,
     0x11, 0x93,
     0x12, 0x03,
     0x15, 0x34,
//     0x17, 0x0F,
     0x17, 0x00, // always go into IDLE
     0x18, 0x18,
     0x19, 0x16,
     0x1B, 0x43,
     0x1C, 0x68,
     0x25, 0x00,
     0x26, 0x11,
     0xff
};

void
rf_rwe_init(void)
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
  for (uint8_t i = 0; i<60; i += 2) {
       
    if (pgm_read_byte( &RWE_CFG[i] )>0x40)
      break;

    cc1100_writeReg( pgm_read_byte(&RWE_CFG[i]),
                     pgm_read_byte(&RWE_CFG[i+1]) );
  }
  
  ccStrobe( CC1100_SCAL );

  my_delay_ms(1);
  checkFrequency(); 
}

void
rf_rwe_task(void)
{
  uint8_t enc[MAX_RWE_MSG];
  uint8_t rssi;

#ifndef USE_RF_MODE
  if(!rwe_on)
    return;
#endif

  // see if a CRC OK pkt has been arrived
#ifdef USE_HAL
  if (hal_CC_Pin_Get(CC_INSTANCE,CC_Pin_In)) {
#else
  if (bit_is_set( CC1100_IN_PORT, CC1100_IN_PIN )) {
#endif

    enc[0] = cc1100_readReg( CC1100_RXFIFO ) & 0x7f; // read len

    if (enc[0]>=MAX_RWE_MSG)
         enc[0] = MAX_RWE_MSG-1;
    
    CC1100_ASSERT;
    cc1100_sendbyte( CC1100_READ_BURST | CC1100_RXFIFO );
    
    for (uint8_t i=0; i<enc[0]; i++) {
         enc[i+1] = cc1100_sendbyte( 0 );
    }
    
    rssi = cc1100_sendbyte( 0 );
    
    CC1100_DEASSERT;

    ccStrobe( CC1100_SFRX  );
    ccStrobe( CC1100_SIDLE );
    ccStrobe( CC1100_SNOP  );
    ccStrobe( CC1100_SRX   );

    MULTICC_PREFIX();

    if(TX_REPORT & REP_BINTIME) {
      
      DC('w');
      for (uint8_t i=0; i<=enc[0]; i++)
      DC( enc[i] );
         
    } else {
      DC('W');
      
      for (uint8_t i=0; i<=enc[0]; i++)
        DH2( enc[i] );

      if (TX_REPORT & REP_RSSI)
        DH2(rssi);
      
      DNL();
    }

    return;
       
  }
       
       
  switch (cc1100_readReg( CC1100_MARCSTATE )) {
            
       // RX_OVERFLOW
  case 17:
       // IDLE
  case 1:
    ccStrobe( CC1100_SFRX  );
    ccStrobe( CC1100_SIDLE );
    ccStrobe( CC1100_SNOP  );
    ccStrobe( CC1100_SRX   );
    break;
       
  }
}

void
rwe_send(char *in)
{
  uint8_t dec[MAX_RWE_MSG];

  uint8_t hblen = fromhex(in+1, dec, MAX_RWE_MSG-1);

  if ((hblen-1) != dec[0]) {
//    DS_P(PSTR("LENERR\r\n"));
    return;
  }

#ifdef USE_RF_MODE
  change_RF_mode(RF_mode_rwe);
#else
  // in Moritz mode already?
  if(!rwe_on) {
    rf_rwe_init();
    my_delay_ms(3);             // 3ms: Found by trial and error
  }
#endif

  ccStrobe(CC1100_SIDLE);

  // load configuration
  for (uint8_t i = 0; i<60; i += 2) {

    if (pgm_read_byte( &RWE_CFG[i] )>0x40)
      break;

    cc1100_writeReg( pgm_read_byte(&RWE_CFG[i]),
                     pgm_read_byte(&RWE_CFG[i+1]) );
  }

  ccStrobe(CC1100_SFTX);

  /* start sending here already - see below why */
  ccStrobe(CC1100_STX);
	
  // loop for a while > tEvent0 (this needs to be reviewed)  
  for (uint8_t i = 0; i<5; i++)
    my_delay_ms(100);

  /* 
     until the TX FIFO is empty the chip is continuesly sending a preamble,
     this will keep the receiver listening after it periodically woken up
     see: http://e2e.ti.com/support/low_power_rf/f/156/t/142864.aspx
  */

  // send
  CC1100_ASSERT;
  cc1100_sendbyte(CC1100_WRITE_BURST | CC1100_TXFIFO);

  for(uint8_t i = 0; i < hblen; i++) {
    cc1100_sendbyte(dec[i]);
  }

  CC1100_DEASSERT;

  while( cc1100_readReg( CC1100_MARCSTATE ) != 1 )
    my_delay_ms(5);
  
  ccStrobe(CC1100_SIDLE);
#ifdef USE_RF_MODE
  if(!restore_RF_mode()) {
    ccRX();
  }
#else
  if(rwe_on) {
    ccRX();
  } else {
    set_txrestore();
  }
#endif
}

void
rwe_func(char *in)
{
  if(in[1] == 'r') {                // Reception on
#ifdef USE_RF_MODE
    set_RF_mode(RF_mode_rwe);
#else
    rf_rwe_init();
    rwe_on = 1;
#endif

  } else if(in[1] == 's') {         // Send
//    rwe_send(in+1);

  } else {                          // Off
#ifdef USE_RF_MODE
    set_RF_mode(RF_mode_off);
#else
    rwe_on = 0;
#endif

  }
}

#endif
