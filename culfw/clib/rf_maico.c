#include "board.h"
#ifdef HAS_MAICO
#include <string.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include "cc1100.h"
#include "delay.h"
#include "rf_receive.h"
#include "display.h"
#include "clock.h"
#include "rf_send.h" //credit_10ms

#include "rf_maico.h"

#include <utility/trace.h>


#ifdef CC1100_MAICO
#include "fncollection.h"

#define CC_ID				CC1100_MAICO

#undef CC1100_ASSERT
#undef CC1100_DEASSERT
#define CC1100_DEASSERT                 hal_CC_Pin_Set(CC_ID,CC_Pin_CS,GPIO_PIN_SET)
#define CC1100_ASSERT                   hal_CC_Pin_Set(CC_ID,CC_Pin_CS,GPIO_PIN_RESET)
#define CC1100_READREG(x)               cc1100_readReg2(x,CC_ID)
#define CC1100_WRITEREG(x,y)            cc1100_writeReg2(x,y,CC_ID)
#define CCSTROBE(x)                     ccStrobe2(x,CC_ID)
#define CC1101_RX_CHECK_PLL_WAIT_TASK() cc1101_RX_check_PLL_wait_task2(CC_ID)

#else
#define CC_ID                           0
#define CC1100_READREG                  cc1100_readReg
#define CC1100_WRITEREG                 cc1100_writeReg
#define CCSTROBE                        ccStrobe

#endif

uint8_t maico_on = 0;


//07 0e
const uint8_t PROGMEM MAICO_CFG[] = {
  0x00, 0x07,
  0x02, 0x2e,
  0x06, 0x0B,
  0x07, 0x0C,
  0x08, 0x44,
  0x09, 0x03,
  0x0a, 0x00,
  0x0B, 0x08,
  0x0c, 0x00,
  0x0D, 0x21,
  0x0E, 0x65,
  0x0F, 0x6A,
  0x10, 0xC9,
  0x11, 0xE4,
  0x12, 0x0B,
  0x13, 0x22,
  0x14, 0xF8,
  0x15, 0x40,
  0x16, 0x07,
  0x17, 0x33, // go into RX after TX, CCA; EQ3 uses 0x03
  0x18, 0x1c,
  0x19, 0x16,
  0x1B, 0x43,
  0x1c, 0x40,
  0x1d, 0x91,
  0x20, 0x38,
  0x21, 0x56,
  0x22, 0x10,
  0x23, 0xE9,
  0x24, 0x2A,
  0x25, 0x00,
  0x26, 0x1F,
  0x29, 0x59,
  0x2c, 0x81,
  0x2D, 0x35,
  0x2E, 0x09,
  0x3e, 0xc3
};

void
rf_maico_init(void)
{
#ifdef ARM
  hal_CC_GDO_init(CC_ID,INIT_MODE_OUT_CS_IN);
  hal_enable_CC_GDOin_int(CC_ID,FALSE); // disable INT - we'll poll...
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

#if (CC_ID != 0)
  CC1100_ASSERT;
  uint8_t *cfg = EE_CC1100_CFG;
  for(uint8_t i = 0; i < EE_CC1100_CFG_SIZE; i++) {
      cc1100_sendbyte(erb(cfg++));
  }
  CC1100_DEASSERT;
#endif

  // load configuration
  for (uint8_t i = 0; i<60; i += 2) {
    if (pgm_read_byte( &MAICO_CFG[i] )>0x40)
      break;

    CC1100_WRITEREG( pgm_read_byte(&MAICO_CFG[i]),
                     pgm_read_byte(&MAICO_CFG[i+1]) );
  }

  CCSTROBE( CC1100_SCAL );

  my_delay_ms(4); // 4ms: Found by trial and error
  //This is ccRx() but without enabling the interrupt
  uint8_t cnt = 0xff;
  //Enable RX. Perform calibration first if coming from IDLE and MCSM0.FS_AUTOCAL=1.
  //Why do it multiple times?
  while(cnt-- && (CCSTROBE( CC1100_SRX ) & 0x70) != 1)
    my_delay_us(10);

  maico_on = 1;
}


void
rf_maico_task(void)
{
  uint8_t msg[MAX_MAICO_MSG];
  uint8_t rssi;

  if(!maico_on)
    return;

  // see if a CRC OK pkt has been arrived
#ifdef ARM
  if (hal_CC_Pin_Get(CC_ID,CC_Pin_In)) {
#else
  if(bit_is_set( CC1100_IN_PORT, CC1100_IN_PIN )) {
#endif
    //errata #1 does not affect us, because we wait until packet is completely received
    msg[0] = CC1100_READREG( CC1100_READ_BURST | CC1100_RXBYTES ) -2; // len

    CC1100_ASSERT;
    cc1100_sendbyte( CC1100_READ_BURST | CC1100_RXFIFO );

    for (uint8_t i=0; i<msg[0]; i++) {
         msg[i+1] = cc1100_sendbyte( 0 );
    }

    // RSSI is appended to RXFIFO
    rssi = cc1100_sendbyte( 0 );
    // And Link quality indicator, too
    /* LQI = */ cc1100_sendbyte( 0 );

    CC1100_DEASSERT;

    if (tx_report & REP_BINTIME) {

      DC('l');
      for (uint8_t i=0; i<=msg[0]; i++)
      DC( msg[i] );
    } else {
      DC('L');
      for (uint8_t i=0; i<=msg[0]; i++)
        DH2( msg[i] );
      if (tx_report & REP_RSSI)
        DH2(rssi);
      DNL();
    }

    return;
  }

  switch(CC1100_READREG( CC1100_MARCSTATE )) {
    case MARCSTATE_RXFIFO_OVERFLOW:
      CCSTROBE( CC1100_SFRX  );
    case MARCSTATE_IDLE:
      CCSTROBE( CC1100_SIDLE );
      CCSTROBE( CC1100_SNOP  );
      CCSTROBE( CC1100_SRX   );
      //TRACE_INFO("Maico CC restart");
      break;
  }
}


void
maico_sendraw(uint8_t *dec)
{
  uint8_t hblen = 0x0b;
  //1kb/s = 1 bit/ms. we send 1 sec preamble + hblen*8 bits
  uint32_t sum = (hblen*8)/10;
  if (credit_10ms < sum) {
    DS_P(PSTR("LOVF\n\r"));
    return;
  }
  credit_10ms -= sum;

  // in Maico mode already?
  if(!maico_on) {
    rf_maico_init();
  }

  if(CC1100_READREG( CC1100_MARCSTATE ) != MARCSTATE_RX) { //error
    DC('Z');
    DC('E');
    DC('R');
    DC('R');
    DC('1');
    DH2(CC1100_READREG( CC1100_MARCSTATE ));
    DNL();
    rf_maico_init();
    return;
  }

  // send
  CC1100_ASSERT;
  cc1100_sendbyte(CC1100_WRITE_BURST | CC1100_TXFIFO);

  for(uint8_t i = 0; i < hblen; i++) {
    cc1100_sendbyte(dec[i]);
    DH2( dec[i] );
  }

  CC1100_DEASSERT;

  // enable TX, wait for CCA
    do {
      CCSTROBE(CC1100_STX);
    } while (CC1100_READREG(CC1100_MARCSTATE) != MARCSTATE_TX);


  //Wait for sending to finish (CC1101 will go to RX state automatically
  //after sending
  uint8_t i;
  for(i=0; i< 200;++i) {
    if( CC1100_READREG( CC1100_MARCSTATE ) == MARCSTATE_RX)
      break; //now in RX, good
    if( CC1100_READREG( CC1100_MARCSTATE ) != MARCSTATE_TX)
      break; //neither in RX nor TX, probably some error
    my_delay_ms(1);
  }

  if(CC1100_READREG( CC1100_MARCSTATE ) != MARCSTATE_RX) { //error
    DC('Z');
    DC('E');
    DC('R');
    DC('R');
    DC('3');
    DH2(CC1100_READREG( CC1100_MARCSTATE ));
    DNL();
    rf_maico_init();
  }

  if(!maico_on) {
    set_txrestore();
  }
}

void
maico_func(char *in)
{
  if(in[1] == 'r') {                // Reception on
    rf_maico_init();

  } else if(in[1] == 's' ) {         // Send/Send fast
    uint8_t dec[0x0b];
    uint8_t hblen = fromhex(in+2, dec, 0x0b);
    if ((hblen) != 0x0b) {
      DS_P(PSTR("LENERR\n\r"));
      TRACE_INFO("LENERR %02X\n\r", hblen);
      return;
    }
    maico_sendraw(dec);

  } else {                          // Off
    maico_on = 0;

  }
}

#endif
