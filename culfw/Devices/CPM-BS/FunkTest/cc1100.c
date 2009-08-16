
#include <stdio.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "cc1100.h"

#define NO_FREQ433 1

const uint8_t CC1100_PA[8] = {
     0x00,
     0x32,
     0x38,
     0x3f,
     0x3f,
     0x3f,
     0x3f,
     0x3f
};

const uint8_t CC1100_CFG[0x2f] = {
  //PROGMEM prog_uint8_t CC1100_CFG[0x29] = {
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

#ifdef FREQ433
     0x10, // 0D FREQ2 x    Fcarrier = Fosc/65536*(FREQ2.FREQ1.FREQ0)
     0xb0, // 0E FREQ1 x    If Fcarrier = 868.36MHz => Fxosc = 26MHz, 
     0x71, // 0F FREQ0 x
#else
     0x21, // 0D FREQ2 x    Fcarrier = Fosc/65536*(FREQ2.FREQ1.FREQ0)
     0x65, // 0E FREQ1 x    If Fcarrier = 868.36MHz => Fxosc = 26MHz, 
     0xe8, // 0F FREQ0 x
#endif

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

     0x05, // 1B AGCCTRL2   MAGN_TARGET = 42db,
     0x00, // 1C AGCCTRL1   No rel tresh., abs tresh. at MAGN_TARGET
     0x90, // 1D AGCCTRL0   Medium hysteresis, #filter: 16, OOK diff 0/1:8dB

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


uint8_t cc1100_sendbyte(uint8_t data) {

     USIDR = data;		        // send byte

     USISR = _BV( USIOIF );

     do {
	  
	  USICR = _BV( USIWM0 ) | _BV(USICS1) | _BV(USICLK) | _BV(USITC); 
	  
     } while (!bit_is_set(USISR, USIOIF));
     
     return USIDR;
}

uint8_t cc1100_read(uint8_t data) {

     CC1100_ASSERT;

     cc1100_sendbyte( data );
     uint8_t temp = cc1100_sendbyte( 0xaa );
     
     CC1100_DEASSERT;
     
     return temp;
}

uint8_t cc1100_readReg(uint8_t addr) {
     return cc1100_read( addr | CC1100_READ_BURST );
}

void ccStrobe(uint8_t strobe) {

     CC1100_ASSERT;
     cc1100_sendbyte( strobe );
     CC1100_DEASSERT;
     
}


//----------------------------------------------------------------------------------
//  void ccResetChip(void)
//
//  DESCRIPTION:
//    Resets the chip using the procedure described in the datasheet.
//----------------------------------------------------------------------------------
void ccResetChip(void) {

//     printf_P( PSTR("Reset CC1100 chip ...\r\n" ) );
     
     // Toggle chip select signal
     CC1100_DEASSERT;
     _delay_us(30);
     CC1100_ASSERT;
     _delay_us(30);
     CC1100_DEASSERT;
     _delay_us(45);
     
     // Send SRES command
     ccStrobe( CC1100_SRES );

     _delay_us(100);

     // load configuration
     CC1100_ASSERT;
     cc1100_sendbyte( 0 | CC1100_WRITE_BURST );

     for (uint8_t i = 0;i<0x29;i++) {
	  cc1100_sendbyte( CC1100_CFG[i] );
     }
     
     CC1100_DEASSERT;

     // setup PA table
     CC1100_ASSERT;
     cc1100_sendbyte( CC1100_PATABLE | CC1100_WRITE_BURST );

     for (uint8_t i = 0;i<8;i++) {
	  cc1100_sendbyte( CC1100_PA[i] );
     }
     
     CC1100_DEASSERT;

     ccStrobe( CC1100_SCAL );
}

void ccInitChip(void) {

//     printf_P( PSTR("Init CC1100 chip ...\r\n" ) );

     SET_BIT( CC1100_CS_DDR, CC1100_CS_PIN ); // CS as output
     
     CC1100_DEASSERT;

     ccResetChip();

//   ccDump();
}


void ccDump(void) {

     /*

     printf_P( PSTR("\r\nPARTNUM   : %d\r\n" ), cc1100_readReg( CC1100_PARTNUM ) );
     printf_P( PSTR("VERSION   : %d\r\n" ), cc1100_readReg( CC1100_VERSION ) );
     printf_P( PSTR("FREQEST   : %d\r\n" ), cc1100_readReg( CC1100_FREQEST ) );
     printf_P( PSTR("LQI       : %d\r\n" ), cc1100_readReg( CC1100_LQI ) );
     printf_P( PSTR("RSSI      : %d\r\n" ), cc1100_readReg( CC1100_RSSI ) );
     printf_P( PSTR("MARCSTATE : %d\r\n" ), cc1100_readReg( CC1100_MARCSTATE ) );
     printf_P( PSTR("WORTIME   : %d\r\n" ), cc1100_readReg( CC1100_WORTIME1 )<<8 | cc1100_readReg( CC1100_WORTIME0 ) );
     printf_P( PSTR("PKTSTATUS : 0x%X\r\n" ), cc1100_readReg( CC1100_PKTSTATUS ) );
     printf_P( PSTR("VCO_VC_DAC: %d\r\n" ), cc1100_readReg( CC1100_VCO_VC_DAC ) );
     printf_P( PSTR("TXBYTES   : %d\r\n" ), cc1100_readReg( CC1100_TXBYTES ) );
     printf_P( PSTR("RXBYTES   : %d\r\n" ), cc1100_readReg( CC1100_RXBYTES ) );

     */

}

