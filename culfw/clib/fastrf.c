#include "board.h"
#include <string.h>
#include <avr/pgmspace.h>
#include "fastrf.h"
#include "cc1100.h"
#include "delay.h"
#include "display.h"
#include "rf_receive.h"
#include "fncollection.h"
#include "clock.h"

#ifdef HAS_FASTRF
uint8_t fastrf_on;

void
fastrf_mode(uint8_t on)
{
  if(on) {
    ccInitChip(EE_FASTRF_CFG);
    ccRX();
    fastrf_on = 1;
  } else {
    fastrf_on = 0;
  }
}

void
fastrf_func(char *in)
{
  uint8_t len = strlen(in);

  if(in[1] == 'r') {                // Init
    fastrf_mode(FASTRF_MODE_ON);

  } else if(in[1] == 's') {         // Send

    CC1100_ASSERT;
    cc1100_sendbyte(CC1100_WRITE_BURST | CC1100_TXFIFO);
    cc1100_sendbyte( len-2 );
    for(uint8_t i=2; i< len; i++)
      cc1100_sendbyte(in[i]);
    CC1100_DEASSERT;
    ccTX();
    while(cc1100_readReg(CC1100_TXBYTES) & 0x7f) // Wait for the data to be sent
      my_delay_us(100);
    ccRX();                         // set reception again. MCSM1 does not work.

  } else {
    fastrf_on = 0;

  }
}

void
FastRF_Task(void)
{
  if(!fastrf_on)
    return;

  if(fastrf_on == 1) {
    static uint8_t lasttick;         // Querying all the time affects reception.
    if(lasttick != (uint8_t)ticks) {
      if(cc1100_readReg(CC1100_MARCSTATE) == MARCSTATE_RXFIFO_OVERFLOW) {
        ccStrobe(CC1100_SFRX);
        ccRX();
      }
      lasttick = (uint8_t)ticks;
    }
    return;
  }

  uint8_t len = cc1100_readReg(CC1100_RXFIFO);
  uint8_t buf[TTY_BUFSIZE];

  if(len < sizeof(buf)) {
    CC1100_ASSERT;
    cc1100_sendbyte( CC1100_READ_BURST | CC1100_RXFIFO );
    for(uint8_t i=0; i<len; i++)
      buf[i] = cc1100_sendbyte( 0 );
    CC1100_DEASSERT;

    uint8_t i;
    for(i=0; i<len; i++)
      DC(buf[i]);
    DNL();
  }

  fastrf_on = 1;
}

#endif
