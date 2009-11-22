#include "board.h"
#include <string.h>
#include <avr/pgmspace.h>
#include "fastrf.h"
#include "cc1100.h"
#include "delay.h"
#include "display.h"
#include "rf_receive.h"
#include "fncollection.h"

#ifdef HAS_FASTRF
uint8_t fastrf_on;


void
fastrf_func(char *in)
{
  uint8_t len = strlen(in);

  if(in[1] == 'r') {                // Init
    ccInitChip(EE_FASTRF_CFG);
    ccRX();
    fastrf_on = 1;

  } else if(in[1] == 's') {         // Send

    CC1100_ASSERT;
    cc1100_sendbyte(CC1100_WRITE_BURST | CC1100_TXFIFO);
    cc1100_sendbyte( len-2 );
    for(uint8_t i=2; i< len; i++)
      cc1100_sendbyte(in[i]);
    CC1100_DEASSERT;
    ccTX();
    // Wait for the data to be sent
    while(cc1100_readReg(CC1100_TXBYTES) & 0x7f)
      my_delay_us(100);
    ccRX();                    // set reception again. MCSM1 does not work.

  } else {
    fastrf_on = 0;

  }
}

void
FastRF_Task(void)
{
  if(fastrf_on != 2)
    return;

  uint8_t len = cc1100_readReg(CC1100_RXFIFO)+2;
  uint8_t buf[33];

  if(len < sizeof(buf)) {
    CC1100_ASSERT;
    cc1100_sendbyte( CC1100_READ_BURST | CC1100_RXFIFO );
    for(uint8_t i=0; i<len; i++)
      buf[i] = cc1100_sendbyte( 0 );
    CC1100_DEASSERT;

    uint8_t i;
    for(i=0; i<len-2; i++)
      DC(buf[i]);
    DC('.');
    DH2(buf[i++]);
    DH2(buf[i]);
  }

  DNL();
  fastrf_on = 1;
}

#endif
