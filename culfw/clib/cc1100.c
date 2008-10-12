#include <stdio.h>
#include <string.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>

#include "delay.h"
#include "display.h"
#include "fncollection.h"
#include "cc1100.h"

PROGMEM prog_uint8_t CC1100_PA[8] =
//{0x00,0x32,0x38,0x3f,0x3f,0x3f,0x3f,0x3f};    //
{0x00,0x03,0x0F,0x27,0x50,0xC8,0xC3,0xC2};    // +10dB, PA Ramping
//{0x00,0xC2,0x00,0x00,0x00,0x00,0x00,0x00};    // +10dB, no PA Ramping

PROGMEM prog_uint8_t CC1100_CFG[0x29] = {
           //    x -> Used, configuration is relevant
     0x0D, // 00 IOCFG2 x   GDO2 Output pin, asynchron serial data
     0x2E, // 01 IOCFG1     GDO1 high inpedance
     0x2D, // 02 IOCFG0     GDO0 configured as input
     0x07, // 03 FIFOTHR x  FIFO Threshhold  TX:33 / RX:32 bytes
     0xD3, // 04 SYNC1
     0x91, // 05 SYNC0
     0x3D, // 06 PKTLEN
     0x04, // 07 PKTCTRL1
     0x32, // 08 PKTCTRL0 x Async serial mode:GDO0, Infinite Packet len
     0x00, // 09 ADDR
     0x00, // 0A CHANNR x   Channel number
     0x06, // 0B FSCTRL1 x  Fif = Fxosc/1024 * FSCTRL1 (6) -> 152343
     0x00, // 0C FSCTRL0 x  Freq Offset
     0x21, // 0D FREQ2 x    Fcarrier = Fosc/65536*(FREQ2.FREQ1.FREQ0)
     0x65, // 0E FREQ1 x    If Fcarrier = 868.36MHz => Fxosc = 26MHz, 
     0xe8, // 0F FREQ0 x
     0x55, // 10 MDMCFG4 x  BWchannel = Fxosc/(8*(4+1)*2) = 325KHz
     0xe4, // 11 MDMCFG3 x  Rdata = (256+228)*2^5/2^28 *Fxosc = 1500
     0x30, // 12 MDMCFG2 x  011 = ASK/OOK
     0x23, // 13 MDMCFG1 x  Preamble bytes: 4 (?)
     0xb9, // 14 MDMCFG0 x  dFchannel = Fxosc/2^18*(256+185)*2^3 = 350kHz
     0x00, // 15 DEVIATN x  fDev = Fxosc/2^17*(8+0)*2^0 = 1586.914
     0x07, // 16 MCSM2      State machine config
     0x30, // 17 MCSM1      State machine config
     0x18, // 18 MCSM0 x    Autocal from IDLE->RX/TX, CHP_RDY after 150us
     0x14, // 19 FOCCFG x   Freq compens: off, pre sync word:3K, post:K/2 ???
     0x6C, // 1A BSCFG x    BitSync: pre sync:2K,3Kp,Ki/2,Kp, No comp.    ???
     0x07, // 1B AGCCTRL2   MAGN_TARGET = 42db,
     0x00, // 1C AGCCTRL1   No rel tresh., abs tresh. at MAGN_TARGET
     0x91, // 1D AGCCTRL0   Medium hysteresis, #filter: 16, OOK diff 0/1:8dB
     0x87, // 1E WOREVT1
     0x6B, // 1F WOREVT0
     0xF8, // 20 WORCTRL    Wake on radio control
     0x56, // 21 FREND1 x   Default
     0x17, // 22 FREND0 x   PA Table index (for xmit): 0/7 -> PA: 00/3f
     //0x11, // 22 FREND0 x   No PA Ramping
     0xE9, // 23 FSCAL3 x   Freqency Synthesizer Calibration
     0x2A, // 24 FSCAL2 x
     0x00, // 25 FSCAL1 x
     0x1F, // 26 FSCAL0 x
     0x41, // 27 RCCTRL1
     0x00, // 28 RCCTRL0
     /*        
     0x59, // 29 FSTEST x
     0x7F, // 30 PTEST
     0x3F, // 31 AGCTST
     0x81, // 32 TEST2 x
     0x35, // 33 TEST1 x
     0x09  // 34 TEST0 x
     */

};

static uint8_t
cc1100_sendbyte(uint8_t data) {

     SPDR = data;		        // send byte
     
     while (!(SPSR & _BV (SPIF)));	// wait until transfer finished
     
     return SPDR;
}

static uint8_t
cc1100_read(uint8_t data) {

     CC1100_ASSERT;

     cc1100_sendbyte( data );
     uint8_t temp = cc1100_sendbyte( 0 );
     
     CC1100_DEASSERT;
     
     return temp;
}

//--------------------------------------------------------------------
//  void ccInitChip(void)
//
//  DESCRIPTION:
//    Resets the chip using the procedure described in the datasheet.
//---------------------------------------------------------------------
void
ccInitChip(void)
{
  uint8_t *p = EE_START_CC1100;

  SET_BIT( CC1100_CS_DDR, CC1100_CS_PIN ); // CS as output

  CC1100_DEASSERT;                           // Toggle chip select signal
  my_delay_us(30);
  CC1100_ASSERT;
  my_delay_us(30);
  CC1100_DEASSERT;
  my_delay_us(45);

  ccStrobe( CC1100_SRES );                   // Send SRES command
  my_delay_us(100);


  CC1100_ASSERT;                             // load configuration
  cc1100_sendbyte( 0 | CC1100_WRITE_BURST );
  for(uint8_t i = 0;i<0x29;i++) {
   cc1100_sendbyte(eeprom_read_byte(p++));
  }
  CC1100_DEASSERT;


  CC1100_ASSERT;                             // setup PA table
  cc1100_sendbyte( CC1100_PATABLE | CC1100_WRITE_BURST );
  for (uint8_t i = 0;i<8;i++) {
   cc1100_sendbyte(eeprom_read_byte(p++));
  }
  CC1100_DEASSERT;

  ccStrobe( CC1100_SCAL );
  my_delay_ms(1);
}

//--------------------------------------------------------------------
void
cc_factory_reset(void)
{
  uint8_t *p = EE_START_CC1100;

  for(uint8_t i = 0;i<0x29;i++) 
    eeprom_write_byte(p++, __LPM(CC1100_CFG+i));
  for (uint8_t i = 0;i<8;i++)
    eeprom_write_byte(p++, __LPM(CC1100_PA+i));
}

//--------------------------------------------------------------------
void
ccTX(void)
{
  EIMSK  &= ~_BV(CC1100_INT);
  while ((cc1100_readReg( CC1100_MARCSTATE ) & 0x1f) != 19)
       ccStrobe( CC1100_STX );
}

//--------------------------------------------------------------------
void
ccRX(void)
{
  while ((cc1100_readReg( CC1100_MARCSTATE ) & 0x1f) != 13)
       ccStrobe( CC1100_SRX );

  EIFR  |= _BV(CC1100_INTF);
  EIMSK |= _BV(CC1100_INT);
}

//--------------------------------------------------------------------
void
ccreg(char *in)
{
  uint8_t b, out;

  if(fromhex(in+1, &b, 1)) {

    out = cc1100_readReg(b);

    DC('C');                    // prefix
    DH(b,2);                    // register number
    DS_P( PSTR(" = 0x") );
    DH(out,2);                  // result, hex
    DS_P( PSTR(" / ") );
    DU(out,2);                  // result, decimal
    DNL();

  }
}

//--------------------------------------------------------------------
uint8_t
cc1100_readReg(uint8_t addr) {
     return cc1100_read( addr | CC1100_READ_BURST );
}


//--------------------------------------------------------------------
void
ccStrobe(uint8_t strobe) {

     CC1100_ASSERT;
     cc1100_sendbyte( strobe );
     CC1100_DEASSERT;
     
}


