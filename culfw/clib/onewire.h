//*****************************************************************************
// This File is based on the  'ds2482.h'
// From the original Author		: Pascal Stang - Copyright (C) 2004
// Target MCU	: Atmel AVR Series
//
//
// This code is distributed under the GNU Public License
//		which can be found at http://www.gnu.org/licenses/gpl.txt
//
//*****************************************************************************

#ifndef ONEWIRE_H
#define ONEWIRE_H


// constants/macros/typdefs

//AS the AD0 & AD1 of DS2482 are set to GND the I2C Device address is the following
#define DS2482_I2C_ADDR		0x30	//< Base I2C address of DS2482 devices

// DS2482 command defines
#define DS2482_CMD_DRST		0xF0	//< DS2482 Device Reset
#define DS2482_CMD_WCFG		0xD2	//< DS2482 Write Configuration
#define DS2482_CMD_CHSL		0xC3	//< DS2482 Channel Select
#define DS2482_CMD_SRP		0xE1	//< DS2482 Set Read Pointer
#define DS2482_CMD_1WRS		0xB4	//< DS2482 1-Wire Reset
#define DS2482_CMD_1WWB		0xA5	//< DS2482 1-Wire Write Byte
#define DS2482_CMD_1WRB		0x96	//< DS2482 1-Wire Read Byte
#define DS2482_CMD_1WSB		0x87	//< DS2482 1-Wire Single Bit
#define DS2482_CMD_1WT		0x78	//< DS2482 1-Wire Triplet

// DS2482 status register bit defines
#define DS2482_STATUS_1WB	0x01	//< DS2482 Status 1-Wire Busy
#define DS2482_STATUS_PPD	0x02	//< DS2482 Status Presence Pulse Detect
#define DS2482_STATUS_SD	0x04	//< DS2482 Status Short Detected
#define DS2482_STATUS_LL	0x08	//< DS2482 Status 1-Wire Logic Level
#define DS2482_STATUS_RST	0x10	//< DS2482 Status Device Reset
#define DS2482_STATUS_SBR	0x20	//< DS2482 Status Single Bit Result
#define DS2482_STATUS_TSB	0x40	//< DS2482 Status Triplet Second Bit
#define DS2482_STATUS_DIR	0x80	//< DS2482 Status Branch Direction Taken

// DS2482 configuration register bit defines
#define DS2482_CFG_APU		0x01	//< DS2482 Config Active Pull-Up
#define DS2482_CFG_PPM		0x02	//< DS2482 Config Presence Pulse Masking
#define DS2482_CFG_SPU		0x04	//< DS2482 Config Strong Pull-Up
#define DS2482_CFG_1WS		0x08	//< DS2482 Config 1-Wire Speed

// DS2482 channel selection code defines
#define DS2482_CH_IO0		0xF0	//< DS2482 Select Channel IO0

// DS2482 read pointer code defines
#define DS2482_READPTR_SR	0xF0	//< DS2482 Status Register
#define DS2482_READPTR_RDR	0xE1	//< DS2482 Read Data Register
#define DS2482_READPTR_CSR	0xD2	//< DS2482 Channel Selection Register
#define DS2482_READPTR_CR	0xC3	//< DS2482 Configuration Register

#define DS2482_TRUE 1
#define DS2482_FALSE 0

// functions

void onewire_Init(void);

void onewire_HsecTask (void) ;
void onewire_SecTask (void);

int
rndup(double n);//round up a float type and show one decimal place

unsigned char onewire_BusyWait(void);
int onewire_Reset(void);
void onewire_WriteBit(unsigned char data);
void onewire_WriteByte(unsigned char data);
unsigned char onewire_ReadByte(void);
unsigned char onewire_ReadBit(void);
void onewire_StartConversion(void);
int onewire_CheckConversionRunning(void);
int onewire_CheckAllConversionsRunning(void);
void onewire_MatchRom(unsigned char*);
void onewire_FullSearch(void);
unsigned char docrc8(unsigned char value);
void onewire_SearchReset(void);
int onewire_Search(void);
void onewire_ReadROMCodes(void);


void onewire_ReadTemperature(void);

void onewire_func(char *);

int ds2482Init(void);
int ds2482Reset(void);
unsigned char ds2482SendCmd(unsigned char cmd);
unsigned char ds2482SendCmdArg(unsigned char cmd, unsigned char arg);

unsigned char i2cMasterSend(unsigned char deviceAddr, unsigned char length, unsigned char* data);
unsigned char i2cMasterReceive(unsigned char deviceAddr, unsigned char length, unsigned char* data);

#endif
