#include "board.h"
#ifdef HAS_MORITZ
#include <string.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include "cc1100.h"
#include "delay.h"
#include "rf_receive.h"
#include "display.h"
#include "clock.h"

#include "rf_moritz.h"

uint8_t moritz_on = 0;

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
};

void
rf_moritz_init(void)
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
  for (uint8_t i = 0; i<60; i += 2) {
    if (pgm_read_byte( &MORITZ_CFG[i] )>0x40)
      break;

    cc1100_writeReg( pgm_read_byte(&MORITZ_CFG[i]),
                     pgm_read_byte(&MORITZ_CFG[i+1]) );
  }

  ccStrobe( CC1100_SCAL );

  my_delay_ms(4); // 4ms: Found by trial and error
  //This is ccRx() but without enabling the interrupt
  uint8_t cnt = 0xff;
  //Enable RX. Perform calibration first if coming from IDLE and MCSM0.FS_AUTOCAL=1.
  //Why do it multiple times?
  while(cnt-- && (ccStrobe( CC1100_SRX ) & 0x70) != 1)
    my_delay_us(10);
}

void
rf_moritz_task(void)
{
  uint8_t enc[MAX_MORITZ_MSG];
  uint8_t rssi;

  if(!moritz_on)
    return;

  // see if a CRC OK pkt has been arrived
  if(bit_is_set( CC1100_IN_PORT, CC1100_IN_PIN )) {
    //errata #1 does not affect us, because we wait until packet is completely received
    enc[0] = cc1100_readReg( CC1100_RXFIFO ) & 0x7f; // read len

    if (enc[0]>=MAX_MORITZ_MSG)
         enc[0] = MAX_MORITZ_MSG-1;

    CC1100_ASSERT;
    cc1100_sendbyte( CC1100_READ_BURST | CC1100_RXFIFO );

    for (uint8_t i=0; i<enc[0]; i++) {
         enc[i+1] = cc1100_sendbyte( 0 );
    }

    // RSSI is appended to RXFIFO
    rssi = cc1100_sendbyte( 0 );
    // And Link quality indicator, too
    /* LQI = */ cc1100_sendbyte( 0 );

    CC1100_DEASSERT;

    if (tx_report & REP_BINTIME) {

      DC('z');
      for (uint8_t i=0; i<=enc[0]; i++)
      DC( enc[i] );
    } else {
      DC('Z');
      for (uint8_t i=0; i<=enc[0]; i++)
        DH2( enc[i] );
      if (tx_report & REP_RSSI)
        DH2(rssi);
      DNL();
    }

    return;
  }

  if(cc1100_readReg( CC1100_MARCSTATE ) == 17) {
    ccStrobe( CC1100_SFRX  );
    ccStrobe( CC1100_SIDLE );
    ccStrobe( CC1100_SRX   );
  }
}

void
moritz_send(char *in)
{
  /* we are not affected by CC1101 errata #6, because MDMCFG2.SYNC_MODE != 0 */
  uint8_t dec[MAX_MORITZ_MSG];

  uint8_t hblen = fromhex(in+1, dec, MAX_MORITZ_MSG-1);

  if ((hblen-1) != dec[0]) {
//    DS_P(PSTR("LENERR\r\n"));
    return;
  }

  // in Moritz mode already?
  if(!moritz_on) {
    rf_moritz_init();
  }

  /* Enable TX. Perform calibration first if MCSM0.FS_AUTOCAL=1 (this is the case) (takes 809μs)
   * start sending - CC1101 will send preamble continuously until TXFIFO is filled.
   * The preamble will wake up devices. See http://e2e.ti.com/support/low_power_rf/f/156/t/142864.aspx
   * It will not go into TX mode instantly if channel is not clear (see CCA_MODE), thus ccTX tries multiple times */
  ccTX();

  /* Send preamble for 1 sec. Keep in mind that waiting for too long may trigger the watchdog (2 seconds on CUL) */
  for(int i=0;i<10;++i)
    my_delay_ms(100); //arg is uint_8, so loop

  // send
  CC1100_ASSERT;
  cc1100_sendbyte(CC1100_WRITE_BURST | CC1100_TXFIFO);

  for(uint8_t i = 0; i < hblen; i++) {
    cc1100_sendbyte(dec[i]);
  }

  CC1100_DEASSERT;

  if(!moritz_on) {
    set_txrestore();
  }
}

void
moritz_func(char *in)
{
  if(in[1] == 'r') {                // Reception on
    rf_moritz_init();
    moritz_on = 1;

  } else if(in[1] == 's') {         // Send
    moritz_send(in+1);

  } else {                          // Off
    moritz_on = 0;

  }
}

#endif
