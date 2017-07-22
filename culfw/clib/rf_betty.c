#include "board.h"
#ifdef HAS_BETTY
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <string.h>

#include "cc1100.h"
#include "clock.h"
#include "delay.h"
#include "display.h"
#include "rf_betty.h"
#include "rf_receive.h"
#include "rf_send.h" //credit_10ms
#include "stringfunc.h"                 // for fromhex
#include "rf_mode.h"
#include "multi_CC.h"
#ifdef ARM
#include <utility/trace.h>
#else
#define TRACE_DEBUG(...)      { }
#define TRACE_INFO(...)      { }
#endif

#ifdef USE_HAL
#include "hal.h"
#endif

#ifndef USE_RF_MODE
uint8_t betty_on = 0;
#endif

// Deviation = 20.629883
// Base frequency = 433.254913
// Carrier frequency = 433.254913
// Channel number = 0
// Carrier frequency = 433.254913
// Modulated = true
// Modulation format = GFSK
// Manchester enable = false
// Sync word qualifier mode = 30/32 sync word bits detected
// Preamble count = 4
// Channel spacing = 184.478760
// Carrier frequency = 433.254913
// Data rate = 37.4908
// RX filter BW = 203.125000
// Data format = Normal mode
// CRC enable = true
// Whitening = false
// Device address = 1
// Address config = No address check
// CRC autoflush = true
// PA ramping = false
// TX power = 0
// Rf settings for CC1101
const uint8_t PROGMEM BETTY_CFG[] = {
    0x07,  // IOCFG2        GDO2 Output Pin Configuration
    0x2E,  // IOCFG1        GDO1 Output Pin Configuration
    0x2E,  // IOCFG0        GDO0 Output Pin Configuration
    0x47,  // FIFOTHR       RX FIFO and TX FIFO Thresholds
    0xD3,  // SYNC1         Sync Word, High Byte
    0x91,  // SYNC0         Sync Word, Low Byte
    0x3E,  // PKTLEN        Packet Length
    0x1C,  // PKTCTRL1      Packet Automation Control
    0x05,  // PKTCTRL0      Packet Automation Control
    0x01,  // ADDR          Device Address
    0x00,  // CHANNR        Channel Number
    0x06,  // FSCTRL1       Frequency Synthesizer Control
    0x00,  // FSCTRL0       Frequency Synthesizer Control
    0x10,  // FREQ2         Frequency Control Word, High Byte
    0xA9,  // FREQ1         Frequency Control Word, Middle Byte
    0xE5,  // FREQ0         Frequency Control Word, Low Byte
    0x8A,  // MDMCFG4       Modem Configuration
    0x7A,  // MDMCFG3       Modem Configuration
    0x13,  // MDMCFG2       Modem Configuration
    0x22,  // MDMCFG1       Modem Configuration
    0xD1,  // MDMCFG0       Modem Configuration
    0x35,  // DEVIATN       Modem Deviation Setting
    0x04,  // MCSM2         Main Radio Control State Machine Configuration
    0x03,  // MCSM1         Main Radio Control State Machine Configuration
    0x18,  // MCSM0         Main Radio Control State Machine Configuration
    0x16,  // FOCCFG        Frequency Offset Compensation Configuration
    0x6C,  // BSCFG         Bit Synchronization Configuration
    0x43,  // AGCCTRL2      AGC Control
    0x40,  // AGCCTRL1      AGC Control
    0x91,  // AGCCTRL0      AGC Control
    0x46,  // WOREVT1       High Byte Event0 Timeout
    0x50,  // WOREVT0       Low Byte Event0 Timeout
    0x78,  // WORCTRL       Wake On Radio Control
    0x56,  // FREND1        Front End RX Configuration
    0x10,  // FREND0        Front End TX Configuration
    0xE9,  // FSCAL3        Frequency Synthesizer Calibration
    0x2A,  // FSCAL2        Frequency Synthesizer Calibration
    0x00,  // FSCAL1        Frequency Synthesizer Calibration
    0x1F,  // FSCAL0        Frequency Synthesizer Calibration
    0x41,  // RCCTRL1       RC Oscillator Configuration
    0x00,  // RCCTRL0       RC Oscillator Configuration
};


void
rf_betty_init(void)
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
  CC1100_ASSERT;                             // load configuration
  cc1100_sendbyte( 0 | CC1100_WRITE_BURST );
  for(uint8_t i = 0; i < sizeof(BETTY_CFG); i++) {
    cc1100_sendbyte(pgm_read_byte(&BETTY_CFG[i]));
  }
  CC1100_DEASSERT;

  ccStrobe( CC1100_SCAL );

  my_delay_ms(4); // 4ms: Found by trial and error
  //This is ccRx() but without enabling the interrupt
  uint8_t cnt = 0xff;
  //Enable RX. Perform calibration first if coming from IDLE and MCSM0.FS_AUTOCAL=1.
  //Why do it multiple times?
  while(cnt-- && (ccStrobe( CC1100_SRX ) & 0x70) != 1)
    my_delay_us(10);

#ifndef USE_RF_MODE
  betty_on = 1;
#endif
}


void
rf_betty_task(void)
{
  uint8_t msg[256];
  uint8_t rssi;
  uint8_t len;

#ifndef USE_RF_MODE
  if(!betty_on)
    return;
#endif

  // see if a CRC OK pkt has been arrived
#ifdef USE_HAL
  if (hal_CC_Pin_Get(CC_INSTANCE,CC_Pin_In)) {
#else
  if(bit_is_set( CC1100_IN_PORT, CC1100_IN_PIN )) {
#endif

    //errata #1 does not affect us, because we wait until packet is completely received
    len = cc1100_readReg( CC1100_READ_BURST | CC1100_RXBYTES ) -2; // len

    CC1100_ASSERT;
    cc1100_sendbyte( CC1100_READ_BURST | CC1100_RXFIFO );

    for (uint8_t i=0; i<len; i++) {
         msg[i] = cc1100_sendbyte( 0 );
    }

    // RSSI is appended to RXFIFO
    rssi = cc1100_sendbyte( 0 );
    // And Link quality indicator, too
    /* LQI = */ cc1100_sendbyte( 0 );

    CC1100_DEASSERT;

    if(len<3) {
      return;
    }

    TRACE_DEBUG("Betty receive %x\n\r",len);

    MULTICC_PREFIX();

    DC('y');
    for (uint8_t i=0; i<len; i++)
      DH2( msg[i] );
    if (TX_REPORT & REP_RSSI)
      DH2(rssi);
    DNL();

    return;
  }

  switch(cc1100_readReg( CC1100_MARCSTATE )) {
    case MARCSTATE_RXFIFO_OVERFLOW:
      ccStrobe( CC1100_SFRX  );
    case MARCSTATE_IDLE:
      ccStrobe( CC1100_SIDLE );
      ccStrobe( CC1100_SNOP  );
      ccStrobe( CC1100_SRX   );
      //TRACE_INFO("Betty CC restart");
      break;
  }
}
/*

yw00
ys06010101583200

  */
void
betty_sendraw(char *in)
{
  uint8_t msg[MAX_BETTY_MSG];
  uint32_t ts1, ts2;

  uint8_t hblen = fromhex(in+1, msg, MAX_BETTY_MSG-1);

  if ((hblen-1) != msg[0]) {
    return;
  }

#ifdef USE_RF_MODE
  change_RF_mode(RF_mode_betty);
#else
  // in Betty mode already?
  if(!betty_on) {
    rf_betty_init();
  }
#endif

  get_timestamp(&ts1);
    do {
      ccStrobe(CC1100_STX);
      if (cc1100_readReg(CC1100_MARCSTATE) != MARCSTATE_TX) {
        get_timestamp(&ts2);
        if (((ts2 > ts1) && (ts2 - ts1 > 100)) ||
            ((ts2 < ts1) && (ts1 + 100 < ts2))) {
          MULTICC_PREFIX();
          DS_P(PSTR("ERR:CCA\r\n"));
          goto out;
        }
      }
    } while (cc1100_readReg(CC1100_MARCSTATE) != MARCSTATE_TX);

    my_delay_ms(10);

    // send
    CC1100_ASSERT;
    cc1100_sendbyte(CC1100_WRITE_BURST | CC1100_TXFIFO);


    for(uint8_t i = 0; i < hblen; i++) {
      cc1100_sendbyte(msg[i]);
    }

    CC1100_DEASSERT;

    // wait for TX to finish
    while(cc1100_readReg( CC1100_MARCSTATE ) == MARCSTATE_TX);

  out:
    if (cc1100_readReg( CC1100_MARCSTATE ) == MARCSTATE_TXFIFO_UNDERFLOW) {
        ccStrobe( CC1100_SFTX  );
        ccStrobe( CC1100_SIDLE );
        ccStrobe( CC1100_SNOP  );
    }


#ifdef USE_RF_MODE
  restore_RF_mode();
#else
  if(!betty_on) {
    set_txrestore();
  }
#endif
}

/*

yw00

 */
void
betty_send_WOR(uint8_t addr)
{
  uint32_t ts1, ts2;
  uint16_t counter = 0;


#ifdef USE_RF_MODE
  change_RF_mode(RF_mode_betty);
#else
  // in Betty mode already?
  if(!betty_on) {
    rf_betty_init();
  }
#endif

  ccStrobe( CC1100_SIDLE );
  cc1100_writeReg(CC1100_MCSM0, BETTY_CFG[CC1100_MCSM0] & 0x0F);
  ccStrobe( CC1100_SCAL);
  while (cc1100_readReg(CC1100_MARCSTATE) != MARCSTATE_IDLE);

  get_timestamp(&ts1);

  do {

    // send
    CC1100_ASSERT;
    cc1100_sendbyte(CC1100_WRITE_BURST | CC1100_TXFIFO);

    cc1100_sendbyte(1);
    cc1100_sendbyte(addr);

    CC1100_DEASSERT;

    // enable TX, wait for CCA
      do {
        ccStrobe(CC1100_STX);
      } while (cc1100_readReg(CC1100_MARCSTATE) != MARCSTATE_TX);


    //Wait for sending to finish (CC1101 will go to idle state automatically
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
      MULTICC_PREFIX();
      DC('y');
      DC('E');
      DC('R');
      DC('R');
      DC('3');
      DH2(cc1100_readReg( CC1100_MARCSTATE ));
      DNL();
      rf_betty_init();
      break;
    }
    counter++;
    get_timestamp(&ts2);
  } while ((ts2-ts1 < BETTY_WOR_TICKS));

  cc1100_writeReg(CC1100_MCSM0, BETTY_CFG[CC1100_MCSM0]);

#ifdef USE_RF_MODE
  restore_RF_mode();
#else
  if(!betty_on) {
    set_txrestore();
  }
#endif
}

void
betty_func(char *in)
{
  if(in[1] == 'r') {                // Reception on
#ifdef USE_RF_MODE
    set_RF_mode(RF_mode_betty);
#else
    rf_betty_init();
#endif

  } else if(in[1] == 's' ) {         // Send/Send fast
    betty_sendraw(in+1);

  } else if(in[1] == 'w' ) {         // Send WOR
    uint8_t addr ;
    fromhex(in+2, &addr, 1);

    betty_send_WOR(addr);

  } else {                          // Off
#ifdef USE_RF_MODE
    set_RF_mode(RF_mode_off);
#else
    betty_on = 0;
#endif

  }
}

#endif
