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

//void maico_sendraw(uint8_t* buf, int longPreamble);
//void maico_sendAck(uint8_t* enc);
//void maico_handleAutoAck(uint8_t* enc);

#ifdef CC1100_MAICO
#include "fncollection.h"

#define CC_ID				CC1100_MAICO

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

#else

#define CC1100_READREG					cc1100_readReg
#define CC1100_WRITEREG 				cc1100_writeReg
#define CCSTROBE						ccStrobe

#endif

uint8_t maico_on = 0;

/*
 * CC1100_PKTCTRL0.LENGTH_CONFIG = 1 //Variable packet length mode. Packet length configured by the first byte after sync word
 *                 CRC_EN = 1
 *                 PKT_FORMAT = 00 //Use FIFOs
 *                 WHITE_DATA = 0
 * MDMCFG2.SYNC_MODE = 3: 30/32 sync word bits detected
 *        .MANCHESTER_EN = 0
 *        .MOD_FORMAT = 0: 2-FSK
 *        .DEM_DCFILT_OFF = 0
 *
 * EVENT0 = 34667
 * t_Event0 = 750/26Mhz * EVENT0 * 2^(5*WOR_RES) = 1 second
 *
 * One message with 12 payload bytes takes (4 byte preamble + 4 byte sync + 12 byte payload) / 1kbit/s = 160 ms.
 */

/*
const uint8_t PROGMEM MORITZ_CFG[60] = {
//     0x00, 0x08,
     0x00, 0x07, //IOCFG2: GDO2_CFG=7: Asserts when a packet has been received with CRC OK. De-asserts when the first byte is read from the RX FIFO
     0x02, 0x46, //IOCFG0
     0x04, 0xC6, //SYNC1
     0x05, 0x26, //SYNC0
     0x0B, 0x06, //FSCTRL1
     0x10, 0xC8, //MDMCFG4 DRATE_E=8,
     0x11, 0x93, //MDMCFG3 DRATE_M=147, data rate = (256+DRATE_M)*2^DRATE_E/2^28*f_xosc = (9992.599) 1kbit/s (at f_xosc=26 Mhz)
     0x12, 0x03, //MDMCFG2 see above
     0x13, 0x22,  //MDMCFG1 CHANSPC_E=2, NUM_PREAMBLE=2 (4 bytes), FEC_EN = 0 (disabled)
    //0x13, 0x72,  //MDMCFG1 CHANSPC_E=2, NUM_PREAMBLE=7 (24 bytes), FEC_EN = 0 (disabled)
     0x15, 0x34, //DEVIATN
//     0x17, 0x00,
     0x17, 0x3F, //MCSM1: TXOFF=RX, RXOFF=RX, CCA_MODE=3:If RSSI below threshold unless currently receiving a packet
     0x18, 0x28, //MCSM0: PO_TIMEOUT=64, FS_AUTOCAL=2: When going from idle to RX or TX automatically
     0x19, 0x16, //FOCCFG
     0x1B, 0x43, //AGCTRL2
     0x21, 0x56, //FREND1
     0x25, 0x00, //FSCAL1
     0x26, 0x11, //FSCAL0
     0x0D, 0x21, //FREQ2
     0x0E, 0x65, //FREQ1
     0x0F, 0x6A, //FREQ0
     0x07, 0x0C, //PKTCTRL1
     0x16, 0x07, //MCSM2 RX_TIME = 7 (Timeout for sync word search in RX for both WOR mode and normal RX operation = Until end of packet) RX_TIME_QUAL=0 (check if sync word is found)
     0x20, 0xF8, //WORCTRL, WOR_RES=00 (1.8-1.9 sec) EVENT1=7 (48, i.e. 1.333 – 1.385 ms)
     0x1E, 0x87, //WOREVT1 EVENT0[high]
     0x1F, 0x6B, //WOREVT0 EVENT0[low]
     0x29, 0x59, //FSTEST
     0x2C, 0x81, //TEST2
     0x2D, 0x35, //TEST1
     0x3E, 0xC3, //?? Readonly PATABLE?
     0xff
};*/
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

//static uint8_t autoAckAddr[3] = {0, 0, 0};
//static uint8_t fakeWallThermostatAddr[3] = {0, 0, 0};
//static uint32_t lastSendingTicks = 0;

void
rf_maico_init(void)
{
#ifdef SAM7
#ifndef CC_ID
  AT91C_BASE_AIC->AIC_IDCR = 1 << CC1100_IN_PIO_ID;	// disable INT - we'll poll...
#endif

  CC1100_CS_BASE->PIO_PPUER = _BV(CC1100_CS_PIN); 		//Enable pullup
  CC1100_CS_BASE->PIO_OER = _BV(CC1100_CS_PIN);			//Enable output
  CC1100_CS_BASE->PIO_PER = _BV(CC1100_CS_PIN);			//Enable PIO control

#elif defined STM32
  hal_CC_GDO_init(INIT_MODE_OUT_CS_IN);
  hal_enable_CC_GDOin_int(FALSE); // disable INT - we'll poll...
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
/*
  uint8_t *pa = EE_CC1100_PA;
    CC1100_ASSERT;                             // setup PA table
    cc1100_sendbyte( CC1100_PATABLE | CC1100_WRITE_BURST );
    for (uint8_t i = 0;i<8;i++) {
      cc1100_sendbyte(erb(pa++));
    }
    CC1100_DEASSERT;
    */
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
  if(bit_is_set( CC1100_IN_PORT, CC1100_IN_PIN )) {
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

  // We have to keep at least 20 ms of silence between two sends
  // (found out by trial and error). ticks runs at 125 Hz (8 ms per tick),
  // so we wait for 3 ticks.
  // This looks a bit cumbersome but handles overflows of ticks gracefully.

  //if(lastSendingTicks)
  //  while(ticks == lastSendingTicks || ticks == lastSendingTicks+1)
  //    my_delay_ms(1);


 // if(longPreamble) {
  //  // Send preamble for 1 sec. Keep in mind that waiting for too long may trigger the watchdog (2 seconds on CUL)
  //  for(int i=0;i<10;++i)
  //    my_delay_ms(100); //arg is uint_8, so loop
 // }

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
  //lastSendingTicks = ticks;
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
/*
  } else if(in[1] == 'a') {         // Auto-Ack
    fromhex(in+2, autoAckAddr, 3);

  } else if(in[1] == 'w') {         // Fake Wall-Thermostat
    fromhex(in+2, fakeWallThermostatAddr, 3);
*/
  } else {                          // Off
    maico_on = 0;

  }
}

#endif
