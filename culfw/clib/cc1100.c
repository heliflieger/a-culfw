#include <stdio.h>
#include <string.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>

#include "delay.h"
#include "display.h"
#include "fncollection.h"
#include "cc1100.h"

PROGMEM prog_uint8_t CC1100_PA[] = {

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

  0x00,0x32,0x38,0x3f,0x3f,0x3f,0x3f,0x3f
};


PROGMEM prog_uint8_t CC1100_CFG[EE_CC1100_SIZE1] = {
//Active                  Our FT  SmartRF Studio
  0x0D, //*00 IOCFG2      0D  0D *0B // GDO2 as serial output
  0x2E, // 01 IOCFG1      2E  2E  2E
  0x2D, //*02 IOCFG0      2D  2D *0C // GDO0 for input
  0x07, // 03 FIFOTHR     47 *07  47
  0xD3, // 04 SYNC1       D3  D3  D3
  0x91, // 05 SYNC0       91  91  91
  0x3D, // 06 PKTLEN      ff *3D *3D
  0x04, // 07 PKTCTRL1    04  04  04
  0x32, // 08 PKTCTRL0    32  32  32
  0x00, // 09 ADDR        00  00  00
  0x00, // 0A CHANNR      00  00  00
  0x06, // 0B FSCTRL1     06  06  06
  0x00, // 0C FSCTRL0     00  00  00
  0x21, // 0D FREQ2       21  21  21
  0x65, // 0E FREQ1       65  65  65
  0xe8, // 0F FREQ0       E8  e8  e8
  0x55, // 10 MDMCFG4     55  55  55
  0xe4, // 11 MDMCFG3     43 *e4  43
  0x30, //*12 MDMCFG2     30 *30 *B0  // DEM_DCFILT_OFF
  0x23, // 13 MDMCFG1     23  23  23
  0xb9, // 14 MDMCFG0     B9  b9  b9
  0x00, // 15 DEVIATN     00  00  00
  0x07, // 16 MCSM2       07  07  07
  0x30, // 17 MCSM1       30  30  30 
  0x18, // 18 MCSM0       18  18  18
  0x14, // 19 FOCCFG      14  14  14
  0x6C, // 1A BSCFG       6C  6C  6C
  0x07, //*1B AGCCTRL2    07  05 *03 // 42 dB instead of 33dB
  0x00, // 1C AGCCTRL1    40 *00  40
  0x90, //*1D AGCCTRL0    91 *90 *92 // 16 samples instead of 32
  0x87, // 1E WOREVT1     87  87  87
  0x6B, // 1F WOREVT0     6B  6B  6B
  0xF8, // 20 WORCTRL     F8  F8  F8
  0x56, // 21 FREND1      56  56  56
  0x17, // 22 FREND0      17  17  17 // 0x11 for no PA ramping
  0xE9, // 23 FSCAL3      E9  E9  E9   EF
  0x2A, // 24 FSCAL2      2A  2A  2A   2F
  0x00, // 25 FSCAL1      00  00  00   18
  0x1F, // 26 FSCAL0      1F  1F  1F
  0x41, // 27 RCCTRL1     41  41  41
  0x00, // 28 RCCTRL0     00  00  00

/*
Conf1: SmartRF Studio:
  Xtal: 26Mhz, RF out: 0dB, PA ramping, Dev:5kHz, Data:1kHz, Modul: ASK/OOK,
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
  for(uint8_t i = 0; i < sizeof(CC1100_CFG); i++) {
    cc1100_sendbyte(erb(p++));
  }
  CC1100_DEASSERT;


  CC1100_ASSERT;                             // setup PA table
  cc1100_sendbyte( CC1100_PATABLE | CC1100_WRITE_BURST );
  for (uint8_t i = 0;i<8;i++) {
    cc1100_sendbyte(erb(p++));
  }
  CC1100_DEASSERT;

  ccStrobe( CC1100_SCAL );
  my_delay_ms(1);
}

//--------------------------------------------------------------------
void
cc_set_pa(uint8_t idx)
{
  uint8_t *t = EE_START_CC1100+sizeof(CC1100_CFG);
  if(idx > 10)
    idx = 7;
  uint8_t *f = CC1100_PA+idx*8;

  for (uint8_t i = 0;i<8;i++)
    ewb(t++, __LPM(f+i));

  // Correct the FREND0
  ewb(EE_START_CC1100+0x22, (idx > 4 && idx < 10) ? 0x11 : 0x17);
}

//--------------------------------------------------------------------
void
cc_factory_reset(void)
{
  uint8_t *t = EE_START_CC1100;

  for(uint8_t i = 0;i<0x29;i++)
    ewb(t++, __LPM(CC1100_CFG+i));
  cc_set_pa(8);
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

    if(hb == 99) {
      for(uint8_t i = 0; i < sizeof(CC1100_CFG); i++) {
        DH(cc1100_readReg(i), 2);
        if((i&7) == 7)
          DNL();
      }
      DNL();
    } else {
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
