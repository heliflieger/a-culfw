#include "board.h"
#ifdef HAS_ZWAVE
#include <string.h>
#include <avr/pgmspace.h>
#include "cc1100.h"
#include "delay.h"
#include "display.h"
#include "clock.h"
#include "rf_zwave.h"

#define MAX_ZWAVE_MSG 64

void zwave_doSend(uint8_t *msg, uint8_t hblen);

uint8_t zwave_on=0;
uint8_t zwave_100kHz=0; // 0: 40kHz, 1:100kHz
uint8_t zwave_hcid[5];  // HomeId (4byte) + CtrlNodeId (1byte)
uint8_t sMsg[MAX_ZWAVE_MSG];
uint32_t sStamp;

const uint8_t PROGMEM ZWAVE_CFG[] = {
  /* 40k */
  CC1100_IOCFG2,    0x01, // 00 GDO2 pin config:               00:FIFOTHR or end
  CC1100_IOCFG0,    0x2e, // 02 GDO0 pin config:                  2e:three state
  CC1100_FIFOTHR,   0x01, // 03 FIFO Threshhold                   01:RX:8, TX:61
  CC1100_SYNC1,     0xaa, // 04 Sync word, high byte
  CC1100_SYNC0,     0x0f, // 05 Sync word, low byte
  CC1100_PKTLEN,    0xff, // 06 Packet length
  CC1100_PKTCTRL1,  0x00, // 07 Packet automation control         00:no crc/addr
  CC1100_PKTCTRL0,  0x00, // 08 Packet automation control         00:fixlen,fifo
  CC1100_FSCTRL1,   0x06, // 0B Frequency synthesizer control

  CC1100_FREQ2,     0x21, // 0D Frequency control word, high byte 868.4MHz
  CC1100_FREQ1,     0x66, // 0E Frequency control word, middle byte
  CC1100_FREQ0,     0x66, // 0F Frequency control word, low byte
  CC1100_MDMCFG4,   0xca, // 10 Modem configuration               bW 101.5kHz
  CC1100_MDMCFG3,   0x93, // 11 Modem configuration               dr 40kHz
  CC1100_MDMCFG2,   0x06, // 12 Modem configuration               2-FSK/16sync
  CC1100_MDMCFG1,   0x72, // 13 Modem configuration               24 preamble
  CC1100_DEVIATN,   0x35, // 15 Modem deviation setting

  CC1100_MCSM0,     0x18, // 18 Main Radio Cntrl State Machine config
  CC1100_FOCCFG,    0x16, // 19 Frequency Offset Compensation config
  CC1100_AGCCTRL2,  0x03, // 1B AGC control
  CC1100_FSCAL3,    0xe9, // 23 Frequency synthesizer calibration
  CC1100_FSCAL2,    0x2a, // 24 Frequency synthesizer calibration
  CC1100_FSCAL1,    0x00, // 25 Frequency synthesizer calibration
  CC1100_FSCAL0,    0x1f, // 26 Frequency synthesizer calibration
  CC1100_PATABLE,   0x50, // 3E

  /* 100k changes */
#define ZWAVE_100k_SIZE (8*2)
  CC1100_FREQ2,     0x21, // 0D Frequency control word, high byte 869.5 MHz
  CC1100_FREQ1,     0x74, // 0E
  CC1100_FREQ0,     0xAD, // 0F
  CC1100_MDMCFG4,   0x5b, // 10 Modem configuration               bW 325kHz
  CC1100_MDMCFG3,   0xf8, // 11 Modem configuration               dr 100kBaud
  CC1100_MDMCFG2,   0x06, // 12 Modem configuration               GFSK/16sync
  CC1100_MDMCFG1,   0x72, // 13 Modem configuration               24 preamble
  CC1100_DEVIATN,   0x47  // 15 Modem deviation setting           dev:47 kHz ->

};



void
zccRX(void)
{
  ccRX();
#ifdef ARM
  AT91C_BASE_AIC->AIC_IDCR = 1 << CC1100_IN_PIO_ID; // disable INT - we'll poll...
#else
  EIMSK &= ~_BV(CC1100_INT);                 // disable INT - we'll poll...
#endif

}

void
rf_zwave_init(void)
{

#ifdef ARM
  CC1100_CS_BASE->PIO_PPUER = _BV(CC1100_CS_PIN);     //Enable pullup
  CC1100_CS_BASE->PIO_OER = _BV(CC1100_CS_PIN);     //Enable output
  CC1100_CS_BASE->PIO_PER = _BV(CC1100_CS_PIN);     //Enable PIO control
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

  uint8_t len = sizeof(ZWAVE_CFG) - (zwave_100kHz ? 0 : ZWAVE_100k_SIZE);
  for (uint8_t i = 0; i < len; i += 2) {
    cc1100_writeReg( pgm_read_byte(&ZWAVE_CFG[i]),
                     pgm_read_byte(&ZWAVE_CFG[i+1]) );
  }

  ccStrobe( CC1100_SCAL );

  my_delay_ms(4);
  zccRX();
}

uint8_t
zwave_forMe(uint8_t *msg)
{
  if(msg[0] == zwave_hcid[0] &&
     msg[1] == zwave_hcid[1] &&
     msg[2] == zwave_hcid[2] &&
     msg[3] == zwave_hcid[3] &&
     msg[8] == zwave_hcid[4])
    return 1;
  return 0;
}

uint8_t
zwave_ckSum(uint8_t *msg, uint8_t len)
{
  uint8_t cs = 0xff;
  while(len)
    cs ^= msg[--len];
  return cs;
}


void
rf_zwave_task(void)
{
  uint8_t msg[MAX_ZWAVE_MSG];

  if(!zwave_on)
    return;
  if(!bit_is_set( CC1100_IN_PORT, CC1100_IN_PIN ))
    return;

  LED_ON();
  CC1100_ASSERT;
  cc1100_sendbyte( CC1100_READ_BURST | CC1100_RXFIFO );
  for(uint8_t i=0; i<8; i++) { // FIFO RX threshold is 8
     msg[i] = cc1100_sendbyte( 0 ) ^ 0xff;
  }
  CC1100_DEASSERT;

  uint8_t len=msg[7], off=8;
  if(len < 9 || len > MAX_ZWAVE_MSG) {
    rf_zwave_init();
    return;
  }

  cc1100_writeReg(CC1100_PKTLEN, len );
  for(uint8_t mwait=MAX_ZWAVE_MSG-8; mwait > 0; mwait--) {
    my_delay_us(220); // 1 byte @ 40kBaud + 10% margin
    uint8_t flen = cc1100_readReg( CC1100_RXBYTES );
    if(flen == 0)
      continue;
    CC1100_ASSERT;
    cc1100_sendbyte( CC1100_READ_BURST | CC1100_RXFIFO );
    if(off+flen > len) {
      rf_zwave_init();
      return;
    }
    for(uint8_t i=0; i<flen; i++)
      msg[off++] = cc1100_sendbyte( 0 ) ^ 0xff;
    CC1100_DEASSERT;
    if(off >= len)
      break;
  }

  uint8_t isOk = (zwave_on == 'm' ||
                  (zwave_forMe(msg) && zwave_ckSum(msg,len-1)==msg[len-1]));

  if(isOk) {
    DC('z'); 
    for(uint8_t i=0; i<len; i++)
      DH2(msg[i]);
    DNL();
  }

  if(zwave_on == 'r' && isOk) { // ACK
    msg[8] = msg[4]; // src -> target
    msg[4] = zwave_hcid[4]; // src == ctrlId
    msg[5] = 0x03; // ??
    msg[7] = 10;
    msg[9] = zwave_ckSum(msg, 9);
    my_delay_ms(1); // ??
    zwave_doSend(msg, 10);

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
  cc1100_writeReg(CC1100_PKTLEN, hblen);
  ccTX();

  CC1100_ASSERT;
  cc1100_sendbyte(CC1100_WRITE_BURST | CC1100_TXFIFO);
  for(uint8_t i = 0; i < hblen; i++)
    cc1100_sendbyte(msg[i] ^ 0xff);
  CC1100_DEASSERT;

  while(cc1100_readReg( CC1100_MARCSTATE ) == MARCSTATE_TX)
    ;
  cc1100_writeReg(CC1100_PKTLEN, 0xff);

  zccRX();
  LED_OFF();
}

void
zwave_send(char *in)
{
  uint8_t hblen = fromhex(in+2, sMsg, MAX_ZWAVE_MSG);
  zwave_doSend(sMsg, hblen);
}

void
zwave_func(char *in)
{
  if(in[1] == 'r' || in[1] == 'm') {// Reception on: receive or monitor
    zwave_100kHz = (in[2] == '1' ? 1 : 0);
    zwave_on = in[1];
    rf_zwave_init();

  } else if(in[1] == 's') {         // Send
    zwave_send(in);

  } else if(in[1] == 'i') {         // set homeId and ctrlId
    if(in[2]) {
      fromhex(in+2, zwave_hcid, 5);
    } else {
      DC(zwave_on);
      DC(' ');
      DH2(zwave_hcid[0]);
      DH2(zwave_hcid[1]);
      DH2(zwave_hcid[2]);
      DH2(zwave_hcid[3]);
      DC(' ');
      DH2(zwave_hcid[4]);
      DNL();
    }

  } else {                          // Off
    zwave_on = 0;

  }
}

#endif
