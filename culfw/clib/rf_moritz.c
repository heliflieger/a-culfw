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
#include "rf_send.h" //credit_10ms

#include "rf_moritz.h"

void moritz_sendraw(uint8_t* buf, int longPreamble);
void moritz_sendAck(uint8_t* enc);
void moritz_handleAutoAck(uint8_t* enc);

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

static uint8_t autoAckAddr[3] = {0, 0, 0};
static uint8_t fakeWallThermostatAddr[3] = {0, 0, 0};
static uint32_t lastSendingTicks = 0;

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

  moritz_on = 1;
}

void
moritz_handleAutoAck(uint8_t* enc)
{
  /* Debug ouput
  DC('D');
  DC('Z');
  DH2(autoAckAddr[0]);
  DH2(autoAckAddr[1]);
  DH2(autoAckAddr[2]);
  DC('-');
  DH2(enc[0]);
  DH2(enc[3]);
  DH2(enc[7]);
  DH2(enc[8]);
  DH2(enc[9]);
  DNL();
  */

  //Send acks to when required by "spec"
  if((autoAckAddr[0] != 0 || autoAckAddr[1] != 0 || autoAckAddr[2] != 0) /* auto-ack enabled */
      && (
           enc[3] == 0x30 /* type ShutterContactState */
        || enc[3] == 0x40 /* type SetTemperature */
        || enc[3] == 0x50 /* type PushButtonState */
        )
      && enc[7] == autoAckAddr[0] /* dest */
      && enc[8] == autoAckAddr[1]
      && enc[9] == autoAckAddr[2])
    moritz_sendAck(enc);

  if((fakeWallThermostatAddr[0] != 0 || fakeWallThermostatAddr[1] != 0 || fakeWallThermostatAddr[2] != 0) /* fake enabled */
      && enc[0] == 11 /* len */
      && enc[3] == 0x40 /* type SetTemperature */
      && enc[7] == fakeWallThermostatAddr[0] /* dest */
      && enc[8] == fakeWallThermostatAddr[1]
      && enc[9] == fakeWallThermostatAddr[2])
    moritz_sendAck(enc);

  return;
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

    moritz_handleAutoAck(enc);

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
    DS_P(PSTR("LENERR\r\n"));
    return;
  }
  moritz_sendraw(dec, 1);
}

/* longPreamble is necessary for unsolicited messages to wakeup the receiver */
void
moritz_sendraw(uint8_t *dec, int longPreamble)
{
  uint8_t hblen = dec[0]+1;
  //1kb/s = 1 bit/ms. we send 1 sec preamble + hblen*8 bits
  uint32_t sum = (longPreamble ? 100 : 0) + (hblen*8)/10;
  if (credit_10ms < sum) {
    DS_P(PSTR("LOVF\r\n"));
    return;
  }
  credit_10ms -= sum;

  // in Moritz mode already?
  if(!moritz_on) {
    rf_moritz_init();
  }

  if(cc1100_readReg( CC1100_MARCSTATE ) != MARCSTATE_RX) { //error
    DC('Z');
    DC('E');
    DC('R');
    DC('R');
    DC('1');
    DH2(cc1100_readReg( CC1100_MARCSTATE ));
    DNL();
    rf_moritz_init();
    return;
  }

  /* We have to keep at least 20 ms of silence between two sends
   * (found out by trial and error). ticks runs at 125 Hz (8 ms per tick),
   * so we wait for 3 ticks.
   * This looks a bit cumbersome but handles overflows of ticks gracefully.
   */
  if(lastSendingTicks)
    while(ticks == lastSendingTicks || ticks == lastSendingTicks+1)
      my_delay_ms(1);

  /* Enable TX. Perform calibration first if MCSM0.FS_AUTOCAL=1 (this is the case) (takes 809μs)
   * start sending - CC1101 will send preamble continuously until TXFIFO is filled.
   * The preamble will wake up devices. See http://e2e.ti.com/support/low_power_rf/f/156/t/142864.aspx
   * It will not go into TX mode instantly if channel is not clear (see CCA_MODE), thus ccTX tries multiple times */
  ccTX();

  if(cc1100_readReg( CC1100_MARCSTATE ) != MARCSTATE_TX) { //error
    DC('Z');
    DC('E');
    DC('R');
    DC('R');
    DC('2');
    DH2(cc1100_readReg( CC1100_MARCSTATE ));
    DNL();
    rf_moritz_init();
    return;
  }

  if(longPreamble) {
    /* Send preamble for 1 sec. Keep in mind that waiting for too long may trigger the watchdog (2 seconds on CUL) */
    for(int i=0;i<10;++i)
      my_delay_ms(100); //arg is uint_8, so loop
  }

  // send
  CC1100_ASSERT;
  cc1100_sendbyte(CC1100_WRITE_BURST | CC1100_TXFIFO);

  for(uint8_t i = 0; i < hblen; i++) {
    cc1100_sendbyte(dec[i]);
  }

  CC1100_DEASSERT;

  //Wait for sending to finish (CC1101 will go to RX state automatically
  //after sending
  uint8_t i;
  for(i=0; i< 200;++i) {
    if( cc1100_readReg( CC1100_MARCSTATE ) == MARCSTATE_RX)
      break; //now in RX, good
    if( cc1100_readReg( CC1100_MARCSTATE ) != MARCSTATE_TX)
      break; //neither in RX nor TX, probably some error
    my_delay_ms(1);
  }

  if(cc1100_readReg( CC1100_MARCSTATE ) != MARCSTATE_RX) { //error
    DC('Z');
    DC('E');
    DC('R');
    DC('R');
    DC('3');
    DH2(cc1100_readReg( CC1100_MARCSTATE ));
    DNL();
    rf_moritz_init();
  }

  if(!moritz_on) {
    set_txrestore();
  }
  lastSendingTicks = ticks;
}

void
moritz_sendAck(uint8_t* enc)
{
  uint8_t ackPacket[12];
  ackPacket[0] = 11; /* len*/
  ackPacket[1] = enc[1]; /* msgcnt */
  ackPacket[2] = 0; /* flag */
  ackPacket[3] = 2; /* type = Ack */
  for(int i=0;i<3;++i) /* src = enc_dst*/
    ackPacket[4+i] = enc[7+i];
  for(int i=0;i<3;++i) /* dst = enc_src */
    ackPacket[7+i] = enc[4+i];
  ackPacket[10] = 0; /* groupid */
  ackPacket[11] = 0; /* payload */

  my_delay_ms(20); /* by experiments */

  moritz_sendraw(ackPacket, 0);

  //Inform FHEM that we send an autoack
  DC('Z');
  for (uint8_t i=0; i < ackPacket[0]+1; i++)
    DH2( ackPacket[i] );
  if (tx_report & REP_RSSI)
    DH2( 0 ); //fake some rssi
  DNL();
}

void
moritz_func(char *in)
{
  if(in[1] == 'r') {                // Reception on
    rf_moritz_init();

  } else if(in[1] == 's' || in[1] == 'f' ) {         // Send/Send fast
    uint8_t dec[MAX_MORITZ_MSG];
    uint8_t hblen = fromhex(in+2, dec, MAX_MORITZ_MSG-1);
    if ((hblen-1) != dec[0]) {
      DS_P(PSTR("LENERR\r\n"));
      return;
    }
    moritz_sendraw(dec, in[1] == 's');

  } else if(in[1] == 'a') {         // Auto-Ack
    fromhex(in+2, autoAckAddr, 3);

  } else if(in[1] == 'w') {         // Fake Wall-Thermostat
    fromhex(in+2, fakeWallThermostatAddr, 3);

  } else {                          // Off
    moritz_on = 0;

  }
}

#endif
