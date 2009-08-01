#include <string.h>
#include <avr/pgmspace.h>
#include "fastrf.h"
#include "cc1100.h"
#include "delay.h"
#include "display.h"

uint8_t fastrf_on;

PROGMEM prog_uint8_t FASTRF_CFG[] = {
//   Addr  Value
     0x00, 0x07,
     0x03, 0x0D,
     0x04, 0xE9,
     0x05, 0xCA,
     0x07, 0x0C,
     0x0B, 0x06,
     0x0D, 0x21,
     0x0E, 0x65,
     0x0F, 0x6A,
     0x10, 0xC8,
     0x11, 0x93,
     0x12, 0x13,  // GFSK
     0x15, 0x34,
     0x17, 0x00,
     0x18, 0x18,
     0x19, 0x16,
     0x1B, 0x43,
     0x21, 0x56,
     0x25, 0x00,
     0x26, 0x11,
     0x2D, 0x35,
     0x3e, 0xc2,
     0xff
};


void
frf_initChip(void)
{
  SET_BIT( CC1100_CS_DDR, CC1100_CS_PIN ); // CS as output

  CC1100_DEASSERT;                           // Toggle chip select signal
  my_delay_us(30);
  CC1100_ASSERT;
  my_delay_us(30);
  CC1100_DEASSERT;
  my_delay_us(45);

  ccStrobe( CC1100_SRES );                   // Send SRES command
  my_delay_us(100);

  uint8_t i = 0;
  for(;;) {
    uint8_t addr = __LPM(FASTRF_CFG+i);
    uint8_t val  = __LPM(FASTRF_CFG+i+1);
    if(addr == 0xff)
      break;
    CC1100_ASSERT;
    cc1100_sendbyte( addr );
    cc1100_sendbyte( val );
    CC1100_DEASSERT;
    i += 2;
  }

  ccStrobe( CC1100_SCAL );
  my_delay_ms(1);
}

void
fastrf(char *in)
{
  uint8_t len = strlen(in);

  if(in[1] == 's') {                       // Send
    frf_initChip();

    ccStrobe( CC1100_SFRX  );
    ccStrobe( CC1100_SFTX  );

    CC1100_ASSERT;
    cc1100_sendbyte(CC1100_WRITE_BURST | CC1100_TXFIFO);
    cc1100_sendbyte( len-2 );
    for(uint8_t i=2; i< len; i++)
      cc1100_sendbyte(in[i]);
    CC1100_DEASSERT;
    ccStrobe( CC1100_SIDLE );
    ccStrobe( CC1100_SFRX  );
    ccStrobe( CC1100_STX   );

  } else if(in[1] == 'r') {                // receive
    frf_initChip();
    fastrf_on = 1;
    ccStrobe( CC1100_SFRX  );
    ccRX();
    Scheduler_SetTaskMode(FastRF_Task, TASK_RUN);	

  } else {                                 // reset
    fastrf_on = 0;
    Scheduler_SetTaskMode(FastRF_Task, TASK_STOP);	

  }
}

TASK(FastRF_Task)
{
  if(fastrf_on != 2)
    return;

  uint8_t buf[32];
  uint8_t len = cc1100_read(CC1100_READ_SINGLE | CC1100_RXFIFO);

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
