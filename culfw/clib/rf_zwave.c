#include <avr/io.h>                     // for _BV, bit_is_set
#include <stdint.h>                     // for uint8_t, uint16_t, uint32_t

#include "board.h"                      // for CC1100_CS_DDR, etc
#include "led.h"                        // for LED_OFF, LED_ON, SET_BIT
#include "stringfunc.h"                 // for fromhex
#ifdef HAS_ZWAVE
#include <avr/pgmspace.h>               // for pgm_read_byte, PROGMEM

#include "cc1100.h"                     // for cc1100_sendbyte, etc
#include "clock.h"                      // for ticks
#include "delay.h"                      // for my_delay_us, my_delay_ms
#include "display.h"                    // for DC, DH2, DNL
#include "rf_zwave.h"
#include "rf_mode.h"
#include "multi_CC.h"

#ifdef USE_HAL
#include "hal.h"
#endif

#ifdef CUL_V4
#define MAX_ZWAVE_MSG 64        // 1024k SRAM is not enough: no SEC for CUL_V4
#else
#define MAX_ZWAVE_MSG (8+158+2) // 158 == aMacMaxMSDUSizeR3 (G.9959)
#endif

void zwave_doSend(uint8_t *msg, uint8_t hblen);

#define DRATE_40k  '4'
#define DRATE_100k '1'
#define DRATE_9600 '9'

uint8_t zwave_on[NUM_ZWAVE] = {0};
uint8_t zwave_drate[NUM_ZWAVE];
uint8_t zwave_hcid[NUM_ZWAVE][5];  // HomeId (4byte) + CtrlNodeId (1byte)
uint8_t zwave_sMsg[NUM_ZWAVE][MAX_ZWAVE_MSG], zwave_ackState[NUM_ZWAVE]={0}, zwave_sLen[NUM_ZWAVE];
uint32_t zwave_sStamp[NUM_ZWAVE];

// See also: ZAD-12837-1, ITU-G.9959
/*
bWidth:
26000000/(8*(4+0)*2^3) = 101.5   # c -> better reception
26000000/(8*(4+1)*2^1) = 325.0   # 5
26000000/(8*(4+0)*2^1) = 406.2   # 4

dRate:
26000000/(2^28)*(256+147)*(2^10) = 39970
26000000/(2^28)*(256+248)*(2^11) = 99975
26000000/(2^28)*(256+131)*(2^ 9) = 19192

deviation:
26000000/(2^17)*(8+5)*(2^3) = 20629.8
26000000/(2^17)*(8+1)*(2^4) = 28564.4

freq:
26000000/(2^16)*0x216666 = 868399.841
26000000/(2^16)*0x2174ad = 869849.884
26000000/(2^16)*0x216699 = 868420.074

Msg length in bits for 8 bytes payload:
9600: 10+2+9+8+1=30 bytes -> 25ms
40k : 10+2+9+8+1=30 bytes ->  6ms
100k: 40+2+9+8+2=61 bytes -> 4.9ms

*/


const uint8_t PROGMEM ZWAVE_CFG_9600[] = {
  CC1100_IOCFG2,    0x01, // 00 GDO2 pin config:               00:FIFOTHR or end
  CC1100_IOCFG0,    0x2e, // 02 GDO0 pin config:                  2e:three state
  CC1100_FIFOTHR,   0x01, // 03 FIFO Threshhold                   01:RX:8, TX:61
  CC1100_SYNC1,     0x55, // 04 Sync word, high byte
  CC1100_SYNC0,     0xf0, // 05 Sync word, low byte
  CC1100_PKTLEN,    0xff, // 06 Packet length
  CC1100_PKTCTRL1,  0x00, // 07 Packet automation control         00:no crc/addr
  CC1100_PKTCTRL0,  0x00, // 08 Packet automation control         00:fixlen,fifo
  CC1100_FSCTRL1,   0x06, // 0B Frequency synthesizer control

  CC1100_FREQ2,     0x21, // 0D Frequency control word, high byte 868.42MHz
  CC1100_FREQ1,     0x66, // 0E Frequency control word, middle byte
  CC1100_FREQ0,     0x99, // 0F Frequency control word, low byte
  CC1100_MDMCFG4,   0x59, // 10 Modem configuration               bW 325kHz
  CC1100_MDMCFG3,   0x83, // 11 Modem configuration               dr 19200
  CC1100_MDMCFG2,   0x1e, // 12 Modem configuration    Manchester/G-FSK/16sync
  CC1100_MDMCFG1,   0x52, // 13 Modem configuration               preamble 12
  CC1100_DEVIATN,   0x35, // 15 Modem deviation setting           dev:21kHz

  CC1100_MCSM0,     0x18, // 18 Main Radio Cntrl State Machine config
  CC1100_FOCCFG,    0x16, // 19 Frequency Offset Compensation config
  CC1100_AGCCTRL2,  0x03, // 1B AGC control
  CC1100_FSCAL3,    0xe9, // 23 Frequency synthesizer calibration
  CC1100_FSCAL2,    0x2a, // 24 Frequency synthesizer calibration
  CC1100_FSCAL1,    0x00, // 25 Frequency synthesizer calibration
  CC1100_FSCAL0,    0x1f, // 26 Frequency synthesizer calibration
  CC1100_PATABLE,   0x50  // 3E
};

const uint8_t PROGMEM ZWAVE_CFG_40k[] = {
  CC1100_IOCFG2,    0x01, // 00 GDO2 pin config:               00:FIFOTHR or end
  CC1100_IOCFG0,    0x2e, // 02 GDO0 pin config:                  2e:three state
  CC1100_FIFOTHR,   0x01, // 03 FIFO Threshhold                   01:RX:8, TX:61
  CC1100_SYNC1,     0xaa, // 04 Sync word, high byte
  CC1100_SYNC0,     0x0f, // 05 Sync word, low byte               inverted
  CC1100_PKTLEN,    0xff, // 06 Packet length
  CC1100_PKTCTRL1,  0x00, // 07 Packet automation control         00:no crc/addr
  CC1100_PKTCTRL0,  0x00, // 08 Packet automation control         00:fixlen,fifo
  CC1100_FSCTRL1,   0x06, // 0B Frequency synthesizer control

  CC1100_FREQ2,     0x21, // 0D Frequency control word, high byte 868.4MHz
  CC1100_FREQ1,     0x66, // 0E Frequency control word, middle byte
  CC1100_FREQ0,     0x66, // 0F Frequency control word, low byte
  CC1100_MDMCFG4,   0xca, // 10 Modem configuration               bW 101kHz
  CC1100_MDMCFG3,   0x93, // 11 Modem configuration               dr 40k
  CC1100_MDMCFG2,   0x06, // 12 Modem configuration               2-FSK/16sync
  CC1100_MDMCFG1,   0x52, // 13 Modem configuration               preamble 12
  CC1100_DEVIATN,   0x35, // 15 Modem deviation setting           dev:21kHz

  CC1100_MCSM0,     0x18, // 18 Main Radio Cntrl State Machine config
  CC1100_FOCCFG,    0x16, // 19 Frequency Offset Compensation config
  CC1100_AGCCTRL2,  0x03, // 1B AGC control
  CC1100_FSCAL3,    0xe9, // 23 Frequency synthesizer calibration
  CC1100_FSCAL2,    0x2a, // 24 Frequency synthesizer calibration
  CC1100_FSCAL1,    0x00, // 25 Frequency synthesizer calibration
  CC1100_FSCAL0,    0x1f, // 26 Frequency synthesizer calibration
  CC1100_PATABLE,   0x50  // 3E
};

const uint8_t PROGMEM ZWAVE_CFG_100k[] = {
  CC1100_IOCFG2,    0x01, // 00 GDO2 pin config:               00:FIFOTHR or end
  CC1100_IOCFG0,    0x2e, // 02 GDO0 pin config:                  2e:three state
  CC1100_FIFOTHR,   0x01, // 03 FIFO Threshhold                   01:RX:8, TX:61
  CC1100_SYNC1,     0xaa, // 04 Sync word, high byte
  CC1100_SYNC0,     0x0f, // 05 Sync word, low byte               inverted
  CC1100_PKTLEN,    0xff, // 06 Packet length
  CC1100_PKTCTRL1,  0x00, // 07 Packet automation control         00:no crc/addr
  CC1100_PKTCTRL0,  0x00, // 08 Packet automation control         00:fixlen,fifo
  CC1100_FSCTRL1,   0x06, // 0B Frequency synthesizer control

  CC1100_FREQ2,     0x21, // 0D Frequency control word, high byte 869.85 MHz
  CC1100_FREQ1,     0x74, // 0E
  CC1100_FREQ0,     0xAD, // 0F
  CC1100_MDMCFG4,   0x4b, // 10 Modem configuration               bW 406kHz
  CC1100_MDMCFG3,   0xf8, // 11 Modem configuration               dr 100k
  CC1100_MDMCFG2,   0x16, // 12 Modem configuration               GFSK/16sync
  CC1100_MDMCFG1,   0x72, // 13 Modem configuration               24 preamble
  CC1100_DEVIATN,   0x41, // 15 Modem deviation setting           dev:28 kHz

  CC1100_MCSM0,     0x18, // 18 Main Radio Cntrl State Machine config
  CC1100_FOCCFG,    0x16, // 19 Frequency Offset Compensation config
  CC1100_AGCCTRL2,  0x03, // 1B AGC control
  CC1100_FSCAL3,    0xe9, // 23 Frequency synthesizer calibration
  CC1100_FSCAL2,    0x2a, // 24 Frequency synthesizer calibration
  CC1100_FSCAL1,    0x00, // 25 Frequency synthesizer calibration
  CC1100_FSCAL0,    0x1f, // 26 Frequency synthesizer calibration
  CC1100_PATABLE,   0x50  // 3E
};



void
zccRX(void)
{
  ccRX();
#ifdef USE_HAL
  hal_enable_CC_GDOin_int(CC_INSTANCE,FALSE); // disable INT - we'll poll...
#else
  EIMSK &= ~_BV(CC1100_INT);                 // disable INT - we'll poll...
#endif

}

void
rf_zwave_init(void)
{

#ifdef USE_HAL
	hal_CC_GDO_init(CC_INSTANCE,INIT_MODE_IN_CS_IN);
#else
  SET_BIT( CC1100_CS_DDR, CC1100_CS_PIN );
#endif

  CC1100_DEASSERT;                           // Toggle chip select signal
  my_delay_us(30);
  CC1100_ASSERT;
  my_delay_us(30);
  CC1100_DEASSERT;
  my_delay_us(45);

  ccStrobe( CC1100_SRES );
  my_delay_us(100);

  if(zwave_drate[CC_INSTANCE] == DRATE_9600) {
    for (uint8_t i = 0; i < sizeof(ZWAVE_CFG_9600); i += 2)
      cc1100_writeReg( pgm_read_byte(&ZWAVE_CFG_9600[i]),
                       pgm_read_byte(&ZWAVE_CFG_9600[i+1]) );

  } else if(zwave_drate[CC_INSTANCE] == DRATE_40k) {
    for (uint8_t i = 0; i < sizeof(ZWAVE_CFG_40k); i += 2)
      cc1100_writeReg( pgm_read_byte(&ZWAVE_CFG_40k[i]),
                       pgm_read_byte(&ZWAVE_CFG_40k[i+1]) );

  } else if(zwave_drate[CC_INSTANCE] == DRATE_100k) {
    for (uint8_t i = 0; i < sizeof(ZWAVE_CFG_100k); i += 2)
      cc1100_writeReg( pgm_read_byte(&ZWAVE_CFG_100k[i]),
                       pgm_read_byte(&ZWAVE_CFG_100k[i+1]) );

  }

  ccStrobe( CC1100_SCAL );

  my_delay_ms(4);
  zccRX();
}

uint8_t
zwave_forMe(uint8_t *msg)
{
  if(msg[0] == zwave_hcid[CC_INSTANCE][0] &&
     msg[1] == zwave_hcid[CC_INSTANCE][1] &&
     msg[2] == zwave_hcid[CC_INSTANCE][2] &&
     msg[3] == zwave_hcid[CC_INSTANCE][3] &&
     (msg[8] == zwave_hcid[CC_INSTANCE][4] || msg[8] == 0xff))
    return 1;
  return 0;
}

uint8_t
zwave_ckSum_8bit(uint8_t *msg, uint8_t len)
{
  uint8_t cs = 0xff;
  while(len)
    cs ^= msg[--len];
  return cs;
}

uint16_t
zwave_ckSum_16bit(uint8_t *msg, uint8_t len) // CRC16 - CCITT
{
  uint16_t crc = 0x1d0f;

  for(int i=0; i<len; i++) {
    uint16_t x = (crc>>8) ^ msg[i];
    x ^= x>>4;
    crc = (crc<<8) ^ (x<<12) ^ (x<<5) ^ x;
  }
  return crc;
}


void
rf_zwave_task(void)
{
  uint8_t msg[MAX_ZWAVE_MSG];

  if(!zwave_on[CC_INSTANCE])
    return;
  if(zwave_ackState[CC_INSTANCE] && ticks > zwave_sStamp[CC_INSTANCE])
    return zwave_doSend(zwave_sMsg[CC_INSTANCE], zwave_sLen[CC_INSTANCE]);

  #ifdef USE_HAL
  if (!hal_CC_Pin_Get(CC_INSTANCE,CC_Pin_In))
#else
  if(!bit_is_set( CC1100_IN_PORT, CC1100_IN_PIN ))
#endif
    return;

  LED_ON();
  CC1100_ASSERT;
  cc1100_sendbyte( CC1100_READ_BURST | CC1100_RXFIFO );
  for(uint8_t i=0; i<8; i++) { // FIFO RX threshold is 8
     msg[i] = cc1100_sendbyte( 0 );
     if(zwave_drate[CC_INSTANCE] != DRATE_9600)
       msg[i] ^= 0xff;
  }
  CC1100_DEASSERT;

  uint8_t len=msg[7], off=8;
  if(len < 9 || len > MAX_ZWAVE_MSG) {
    rf_zwave_init();
    //DC('l'); DNL();
    return;
  }

  cc1100_writeReg(CC1100_PKTLEN, len );

  // 1 byte @ dataRate + 10% margin
  uint16_t delay = (zwave_drate[CC_INSTANCE] == DRATE_100k ? 90 :
                   (zwave_drate[CC_INSTANCE] == DRATE_40k ? 220 : 920));
  for(uint8_t mwait=MAX_ZWAVE_MSG-8; mwait > 0; mwait--) {
    my_delay_us(delay);
    uint8_t flen = cc1100_readReg( CC1100_RXBYTES );
    if(flen == 0)
      continue;
    if(off+flen > len) {
      //DC('L'); DNL();
      rf_zwave_init();
      return;
    }
    CC1100_ASSERT;
    cc1100_sendbyte( CC1100_READ_BURST | CC1100_RXFIFO );
    for(uint8_t i=0; i<flen; i++) {
      uint8_t d = cc1100_sendbyte( 0 );
      if(zwave_drate[CC_INSTANCE] != DRATE_9600)
        d ^= 0xff;
      msg[off++] = d;
    }
    CC1100_DEASSERT;
    if(off >= len)
      break;
  }

  uint8_t isOk = 0;
  if(zwave_drate[CC_INSTANCE] == DRATE_100k) {
    uint16_t cs = ((uint16_t)msg[len-2]<<8)+msg[len-1]; // Wrong byte order
    isOk = (zwave_ckSum_16bit(msg,len-2) == cs);
  } else {
    isOk = (zwave_ckSum_8bit(msg,len-1) == msg[len-1]);
  }

  if(zwave_on[CC_INSTANCE] == 'r' && !zwave_forMe(msg))
    isOk = 0;

  if(isOk) {
    MULTICC_PREFIX();
    DC('z'); 
    for(uint8_t i=0; i<len; i++)
      DH2(msg[i]);
    DNL();
  } else {
    //DC('C'); DNL();
  }

  if(zwave_on[CC_INSTANCE]=='r' && isOk && (msg[5]&3) == 3 && zwave_ackState[CC_INSTANCE]) // got ACK
    zwave_ackState[CC_INSTANCE] = 0;

  if(zwave_on[CC_INSTANCE]=='r' && isOk && (msg[5]&0x40) &&  // ackReq
     ((msg[5]&0x80) == 0)) {                    // not routed
    my_delay_ms(10); // Tested with 1,5,10,15

    msg[8] = msg[4]; // src -> target
    msg[4] = zwave_hcid[CC_INSTANCE][4]; // src == ctrlId
    msg[5] = 0x03;

    if(zwave_drate[CC_INSTANCE] == DRATE_100k) {
      msg[7] = 11;        // Len
      uint16_t cs = zwave_ckSum_16bit(msg, 9);
      msg[9] = (cs >> 8) & 0xff;
      msg[10] = cs & 0xff;
      zwave_doSend(msg, 11);

    } else {
      msg[7] = 10;        // Len
      msg[9] = zwave_ckSum_8bit(msg, 9);
      zwave_doSend(msg, 10);

    }

    MULTICC_PREFIX();
    DC('z'); DC('a'); DH2(msg[8]); DNL();

  } else {
    cc1100_writeReg(CC1100_PKTLEN, 0xff );
    zccRX();
  }

  LED_OFF();
}

void
zwave_doSend(uint8_t *msg, uint8_t hblen)
{
  LED_ON();

  if (zwave_drate[CC_INSTANCE] == DRATE_9600) {
    cc1100_writeReg(CC1100_MDMCFG2, 0x14);       // No preamble, no manchaster,
    cc1100_writeReg(CC1100_PKTLEN,  2*hblen+19); // we do all this by hand.

  } else {
    cc1100_writeReg(CC1100_PKTLEN, hblen);
  }
  
  ccTX();       // Issues SIDLE, calibrating
  CC1100_ASSERT;
  cc1100_sendbyte(CC1100_WRITE_BURST | CC1100_TXFIFO);
  
  if(zwave_drate[CC_INSTANCE] == DRATE_9600) {
    for(uint8_t i = 0; i < 15; i++) {
      cc1100_sendbyte(0x66);  // preamble 0x55; manchester code = 0x6666
    }
    cc1100_sendbyte(0xaa);    // sync 0xf0; manchester code = 0xaa55
    cc1100_sendbyte(0x55);    //

    for(uint8_t i = 0; i < hblen; i++) {
      uint8_t d = msg[i], m;
      m  = (d & 0x80) ? 0x80 : 0x40;    // manchester encoding
      m += (d & 0x40) ? 0x20 : 0x10;
      m += (d & 0x20) ? 0x08 : 0x04;
      m += (d & 0x10) ? 0x02 : 0x01;
      cc1100_sendbyte(m);
      m  = (d & 0x08) ? 0x80 : 0x40;
      m += (d & 0x04) ? 0x20 : 0x10;
      m += (d & 0x02) ? 0x08 : 0x04;
      m += (d & 0x01) ? 0x02 : 0x01;
      cc1100_sendbyte(m);
    }
    cc1100_sendbyte(0x00);
    cc1100_sendbyte(0x00);

  } else {
    for(uint8_t i = 0; i < hblen; i++)
      cc1100_sendbyte(msg[i] ^ 0xff);
 
  }

  CC1100_DEASSERT;

  while(cc1100_readReg( CC1100_MARCSTATE ) == MARCSTATE_TX)
    ;
  cc1100_writeReg(CC1100_PKTLEN, 0xff);
  if(zwave_drate[CC_INSTANCE] == DRATE_9600) {
    cc1100_writeReg(CC1100_MDMCFG2, 0x1e);
  }

  zccRX();
  LED_OFF();

  if(msg[5] & 0x40) {   // ackReq
    zwave_sStamp[CC_INSTANCE] = ticks + 6; // 6/125 = 48ms
    if(++zwave_ackState[CC_INSTANCE] > 1) {
      MULTICC_PREFIX();
      DC('z'); DC('r'); DH2(zwave_ackState[CC_INSTANCE]); DNL();
    }
    if(zwave_ackState[CC_INSTANCE] >= 3)
      zwave_ackState[CC_INSTANCE] = 0;
  }
}

void
zwave_func(char *in)
{
  if(in[1] == 'r' || in[1] == 'm') {// Reception on: receive or monitor
    zwave_drate[CC_INSTANCE] = (in[2] ? in[2] : DRATE_40k); // Valid: '1', '4', '9'
    zwave_on[CC_INSTANCE] = in[1];
#ifdef USE_RF_MODE
    set_RF_mode(RF_mode_zwave);
#else
    rf_zwave_init();
#endif

  } else if(in[1] == 's') {         // Send
    zwave_ackState[CC_INSTANCE] = 0;
    zwave_sLen[CC_INSTANCE] = fromhex(in+2, zwave_sMsg[CC_INSTANCE], MAX_ZWAVE_MSG);
    zwave_doSend(zwave_sMsg[CC_INSTANCE], zwave_sLen[CC_INSTANCE]);

  } else if(in[1] == 'i') {         // set homeId and ctrlId
    if(in[2]) {
      fromhex(in+2, zwave_hcid[CC_INSTANCE], 5);
    } else {
      MULTICC_PREFIX();
      DC(zwave_on[CC_INSTANCE]);
      DC(zwave_drate[CC_INSTANCE]);
      DC(' ');
      DH2(zwave_hcid[CC_INSTANCE][0]);
      DH2(zwave_hcid[CC_INSTANCE][1]);
      DH2(zwave_hcid[CC_INSTANCE][2]);
      DH2(zwave_hcid[CC_INSTANCE][3]);
      DC(' ');
      DH2(zwave_hcid[CC_INSTANCE][4]);
      DNL();
    }

  } else {                          // Off
    zwave_on[CC_INSTANCE] = 0;
#ifdef USE_RF_MODE
    set_RF_mode(RF_mode_off);
#endif
  }
}

#endif
