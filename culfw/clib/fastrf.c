#include "board.h"
#ifdef HAS_FASTRF
#include <string.h>
#include <avr/pgmspace.h>
#include "fastrf.h"
#include "cc1100.h"
#include "delay.h"
#include "display.h"
#include "rf_receive.h"
#include "fncollection.h"

uint8_t fastrf_on;

PROGMEM prog_uint8_t FASTRF_CFG[EE_FASTRF_CFG_SIZE] = {
// CULFW   IDX NAME     
   0x07, // 00 IOCFG2   
   0x2E, // 01 IOCFG1   
   0x3F, // 02 IOCFG0   
   0x0D, // 03 FIFOTHR  
   0xE9, // 04 SYNC1    
   0xCA, // 05 SYNC0    
   0xFF, // 06 PKTLEN   
   0x0C, // 07 PKTCTRL1 
   0x45, // 08 PKTCTRL0 
   0x00, // 09 ADDR     
   0x00, // 0A CHANNR   
   0x06, // 0B FSCTRL1  
   0x00, // 0C FSCTRL0  
   0x21, // 0D FREQ2    
   0x65, // 0E FREQ1    
   0x6A, // 0F FREQ0    
   0xC8, // 10 MDMCFG4  
   0x93, // 11 MDMCFG3  
   0x13, // 12 MDMCFG2  
   0x22, // 13 MDMCFG1  
   0xF8, // 14 MDMCFG0  
   0x34, // 15 DEVIATN  
   0x07, // 16 MCSM2    
   0x00, // 17 MCSM1    
   0x18, // 18 MCSM0    
   0x16, // 19 FOCCFG   
   0x6C, // 1A BSCFG    
   0x43, // 1B AGCCTRL2 
   0x40, // 1C AGCCTRL1 
   0x91, // 1D AGCCTRL0 
   0x87, // 1E WOREVT1  
   0x6B, // 1F WOREVT0  
   0xF8, // 20 WORCTRL  
   0x56, // 21 FREND1   
   0x11, // 22 FREND0  *
   0xA9, // 23 FSCAL3   
   0x0A, // 24 FSCAL2  *
   0x00, // 25 FSCAL1  *
   0x11, // 26 FSCAL0   
   0x41, // 27 RCCTRL1  
   0x00, // 28 RCCTRL0  
};

void
fastrf(char *in)
{
  uint8_t len = strlen(in);

  if(in[1] == 's') {                       // Send

    ccInitChip(EE_FASTRF_CFG, EE_CC1100_PA);
    ccStrobe( CC1100_SIDLE );
    ccStrobe( CC1100_SFTX  );
    CC1100_ASSERT;
    cc1100_sendbyte(CC1100_WRITE_BURST | CC1100_TXFIFO);
    cc1100_sendbyte( len-2 );
    for(uint8_t i=2; i< len; i++)
      cc1100_sendbyte(in[i]);
    CC1100_DEASSERT;
    ccStrobe( CC1100_STX   );

  } else if(in[1] == 'r') {                // receive
    ccInitChip(EE_FASTRF_CFG, EE_CC1100_PA);
    fastrf_on = 1;
    ccStrobe( CC1100_SFRX  );
    ccRX();

  } else {                                 // reset
    fastrf_on = 0;
    set_txrestore();

  }
}

void
FastRF_Task(void)
{
  if(fastrf_on != 2)
    return;

  uint8_t buf[32];
  uint8_t len = cc1100_readReg(CC1100_RXFIFO);

  if(len < sizeof(buf)) {
    DC('f');
    CC1100_ASSERT;
    cc1100_sendbyte( CC1100_READ_BURST | CC1100_RXFIFO );
    for(uint8_t i=0; i<len; i++)
      buf[i] = cc1100_sendbyte( 0 );
    CC1100_DEASSERT;

    for(uint8_t i=0; i<len; i++)
      DC(buf[i]);
    DNL();
  }
  ccStrobe( CC1100_SIDLE  );
  ccStrobe( CC1100_SFRX  );
  ccRX();

  fastrf_on = 1;
}

//--------------------------------------------------------------------
void
fastrf_reset(void)
{
  uint8_t *t = EE_FASTRF_CFG;
  for(uint8_t i = 0; i < sizeof(FASTRF_CFG); i++)
    ewb(t++, __LPM(FASTRF_CFG+i));

#ifdef BUSWARE_CUL
  // check 433MHz version marker and patch default frequency
  if (!bit_is_set(PINB, PB6)) {
       t = EE_FASTRF_CFG + 0x0d;
       ewb(t++, 0x10);
       ewb(t++, 0xb0);
       ewb(t++, 0x71);
  }
#endif
}


#endif
