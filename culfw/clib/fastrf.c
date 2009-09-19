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

#if 0
PROGMEM prog_uint8_t FASTRF_CFG[EE_FASTRF_CFG_SIZE] = { // 10kHz GFSK
// CULFW   IDX NAME     
   0x07, // 00 IOCFG2   
   0x2E, // 01 IOCFG1   
   0x3F, // 02 IOCFG0   
   0x0D, // 03 FIFOTHR    TX:9 / RX:56
   0xE9, // 04 SYNC1    
   0xCA, // 05 SYNC0    
   0xFF, // 06 PKTLEN   
   0x0C, // 07 PKTCTRL1 
   0x45, // 08 PKTCTRL0    TX/RX Mode, Whiten, Check CRC, Inf Paket len
   0x00, // 09 ADDR     
   0x00, // 0A CHANNR   
   0x06, // 0B FSCTRL1     152kHz IF Freq
   0x00, // 0C FSCTRL0  
   0x21, // 0D FREQ2       868.3MHz
   0x65, // 0E FREQ1    
   0x6A, // 0F FREQ0    
   0xC8, // 10 MDMCFG4     101.5kHz
   0x93, // 11 MDMCFG3     DRate: 9992 ((256+147)*2^8)*26000000/(2^28)
   0x13, // 12 MDMCFG2     Modulation: GFSK, Sync 32:30
   0x22, // 13 MDMCFG1     Preamble: 4
   0xF8, // 14 MDMCFG0     Channel: 200kHz 26000000*(256+248)*2^2/2^18
   0x34, // 15 DEVIATN  
   0x07, // 16 MCSM2    
   0x00, // 17 MCSM1    
   0x18, // 18 MCSM0       Calibration: RX/TX->IDLE
   0x16, // 19 FOCCFG   
   0x6C, // 1A BSCFG    
   0x43, // 1B AGCCTRL2    No highest gain, 33dB
   0x40, // 1C AGCCTRL1 
   0x91, // 1D AGCCTRL0    16 channel samples
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

#else

// Tried to implement parallel reception of slowrf and fastrf, but it does
// not work: IOCFG2 & PKTCTRL0 must be reprogrammed

PROGMEM prog_uint8_t FASTRF_CFG[EE_FASTRF_CFG_SIZE] = { // 101kHz ASK
// CULFW   IDX NAME        COMMENT
   0x07, // 00 IOCFG2      * GDO2 as serial output   EEPROM 0x37
   0x2E, // 01 IOCFG1      Tri-State
   0x2D, // 02 IOCFG0      GDO0 for input
   0x07, // 03 FIFOTHR     TX:33 TX:32
   0xD3, // 04 SYNC1       
   0x91, // 05 SYNC0       
   0xFF, // 06 PKTLEN    
   0x0C, // 07 PKTCTRL1    No Addr check, Auto FIFO flush, Add Status
   0x45, // 08 PKTCTRL0    TX/RX Mode, Whiten, Check CRC, Inf Paket len
   0x00, // 09 ADDR        
   0x00, // 0A CHANNR      
   0x06, // 0B FSCTRL1     152kHz IF Frquency
   0x00, // 0C FSCTRL0     
   0x21, // 0D FREQ2       868.3MHz
   0x65, // 0E FREQ1       
   0x6a, // 0F FREQ0       
   0x5c, // 10 MDMCFG4     bWidth 325kHz
   0x00, // 11 MDMCFG3     Drate: 101562 ((256+0)*2^12)*26000000/(2^28)
   0x33, // 12 MDMCFG2     Modulation: ASK/OOK, Sync 32:30
   0x23, // 13 MDMCFG1     Preamble: 4
   0xb9, // 14 MDMCFG0     Channel: 350kHz  26000000*(256+185)*2^3/2^18
   0x00, // 15 DEVIATN     
   0x07, // 16 MCSM2       ?
   0x00, // 17 MCSM1       
   0x18, // 18 MCSM0       Calibration: RX/TX->IDLE
   0x14, // 19 FOCCFG      
   0x6C, // 1A BSCFG       
   0x07, // 1B AGCCTRL2    42dB
   0x00, // 1C AGCCTRL1    
   0x90, // 1D AGCCTRL0    4dB decision boundery
   0x87, // 1E WOREVT1     
   0x6B, // 1F WOREVT0     
   0xF8, // 20 WORCTRL     
   0x56, // 21 FREND1      
   0x11, // 22 FREND0      0x11 for no PA ramping
   0xE9, // 23 FSCAL3        
   0x2A, // 24 FSCAL2        
   0x00, // 25 FSCAL1        
   0x1F, // 26 FSCAL0        
   0x41, // 27 RCCTRL1       
   0x00, // 28 RCCTRL0       
                                      
};
#endif

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
