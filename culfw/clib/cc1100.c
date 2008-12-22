#include <stdio.h>
#include <string.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>

#include "delay.h"
#include "display.h"
#include "fncollection.h"
#include "cc1100.h"

PROGMEM prog_uint8_t CC1100_PA[] = {
  //0x00,0x32,0x38,0x3f,0x3f,0x3f,0x3f,0x3f,    // Old

  0x00,0x03,0x0F,0x1E,0x26,0x27,0x00,0x00,      //-10 dBm
  0x00,0x03,0x0F,0x1E,0x25,0x36,0x38,0x67,      // -5 dBm
  0x00,0x03,0x0F,0x25,0x67,0x40,0x60,0x50,      //  0 dBm
  0x00,0x03,0x0F,0x27,0x51,0x88,0x83,0x81,      //  5 dBm
  0x00,0x03,0x0F,0x27,0x50,0xC8,0xC3,0xC2,      // 10 dBm

  0x00,0x27,0x00,0x00,0x00,0x00,0x00,0x00,      //-10 dBm
  0x00,0x67,0x00,0x00,0x00,0x00,0x00,0x00,      // -5 dBm (checked 3 times!)
  0x00,0x50,0x00,0x00,0x00,0x00,0x00,0x00,      //  0 dBm
  0x00,0x81,0x00,0x00,0x00,0x00,0x00,0x00,      //  5 dBm
  0x00,0xC2,0x00,0x00,0x00,0x00,0x00,0x00,      // 10 dBm
};


PROGMEM prog_uint8_t CC1100_CFG[0x29] = {
//  Conf1                 OLD
    0x0D, //*00 IOCFG2    0x0D
    0x2E, // 01 IOCFG1    0x2E
    0x2D, //*02 IOCFG0    0x2D
    0x47, // 03 FIFOTHR   0x07 ?
    0xD3, // 04 SYNC1     0xD3
    0x91, // 05 SYNC0     0x91
    0xff, // 06 PKTLEN    0x3D
    0x04, // 07 PKTCTRL1  0x04
    0x32, // 08 PKTCTRL0  0x32
    0x00, // 09 ADDR      0x00
    0x00, // 0A CHANNR    0x00
    0x06, // 0B FSCTRL1   0x06
    0x00, // 0C FSCTRL0   0x00
    0x21, // 0D FREQ2     0x21
    0x65, // 0E FREQ1     0x65
    0xE8, // 0F FREQ0     0xe8
    0x55, // 10 MDMCFG4   0x55
    0xE4, // 11 MDMCFG3   0xe4
    0xB0, // 12 MDMCFG2   0x30 ?
    0x23, // 13 MDMCFG1   0x23
    0xB9, // 14 MDMCFG0   0xb9
    0x00, // 15 DEVIATN   0x00
    0x07, // 16 MCSM2     0x07
    0x30, // 17 MCSM1     0x30
    0x18, // 18 MCSM0     0x18
    0x14, // 19 FOCCFG    0x14
    0x6C, // 1A BSCFG     0x6C
    0x07, //*1B AGCCTRL2  0x07
    0x40, // 1C AGCCTRL1  0x00 ?
    0x91, //*1D AGCCTRL0  0x91
    0x87, // 1E WOREVT1   0x87
    0x6B, // 1F WOREVT0   0x6B
    0xF8, // 20 WORCTRL   0xF8
    0x56, // 21 FREND1    0x56
    0x17, // 22 FREND0    0x17 // 0x11 for no PA ramping
    0xE9, // 23 FSCAL3    0xE9
    0x2A, // 24 FSCAL2    0x2A
    0x00, // 25 FSCAL1    0x00
    0x1F, // 26 FSCAL0    0x1F
    0x41, // 27 RCCTRL1   0x41
    0x00, // 28 RCCTRL0   0x00

/*
Conf1: SmartRF Studio:
  Xtal: 26Mhz, RF out: 0dB, PA ramping, Dev:5kHz, Data:1.5kHz, Modul: ASK/OOK,
  RF: 868.35MHz, Chan:350kHz, RX Filter: 325kHz
  SimpleRX: Async, SimpleTX: Async+Unmodulated
*/


};

static uint8_t
cc1100_sendbyte(uint8_t data)
{
  SPDR = data;		        // send byte
  while (!(SPSR & _BV (SPIF)));	// wait until transfer finished
  return SPDR;
}

static uint8_t
cc1100_read(uint8_t data)
{
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
cc_set_pa(uint8_t idx)
{
  uint8_t *t = EE_START_CC1100+0x29;
  if(idx > 9)
    idx = 2;
  uint8_t *f = CC1100_PA+idx*8;

  for (uint8_t i = 0;i<8;i++)
    eeprom_write_byte(t++, __LPM(f+i));

  // Correct the FREND0
  eeprom_write_byte(EE_START_CC1100+0x22, (idx > 4) ? 0x11 : 0x17);
}

//--------------------------------------------------------------------
void
cc_factory_reset(void)
{
  uint8_t *t = EE_START_CC1100;

  for(uint8_t i = 0;i<0x29;i++)
    eeprom_write_byte(t++, __LPM(CC1100_CFG+i));
  cc_set_pa(2);
}

//--------------------------------------------------------------------
void
ccsetpa(char *in)
{
  uint8_t hb = 2;
  fromhex(in+1, &hb, 1);
  cc_set_pa(hb);
  ccInitChip();
}

//--------------------------------------------------------------------
void
ccTX(void)
{
  uint8_t cnt = 0xff;
  EIMSK  &= ~_BV(CC1100_INT);

  while (cnt-- && (cc1100_readReg( CC1100_MARCSTATE ) & 0x1f) != 19) {
    ccStrobe( CC1100_STX );
  }
}

//--------------------------------------------------------------------
void
ccRX(void)
{
  uint8_t cnt = 0xff;
  while (cnt-- && (cc1100_readReg( CC1100_MARCSTATE ) & 0x1f) != 13)
       ccStrobe( CC1100_SRX );
  EIFR  |= _BV(CC1100_INT);
  EIMSK |= _BV(CC1100_INT);
}

//--------------------------------------------------------------------
void
ccreg(char *in)
{
  uint8_t hb, out;

  if(fromhex(in+1, &hb, 1)) {

    out = cc1100_readReg(hb);

    DC('C');                    // prefix
    DH(hb,2);                   // register number
    DS_P( PSTR(" = ") );
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
ccStrobe(uint8_t strobe)
{
  CC1100_ASSERT;
  cc1100_sendbyte( strobe );
  CC1100_DEASSERT;
}
