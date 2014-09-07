/***********************************************************************************
    Filename: smode_rf_settings.h
***********************************************************************************/

#ifndef SMODE_RF_SETTINGS
#define SMODE_RF_SETTINGS

#include "cc1100.h"


// Product = CC1101
// Chip version = A   (VERSION = 0x04)
// Crystal accuracy = 10 ppm
// X-tal frequency = 26 MHz
// RF output power = + 10 dBm
// RX filterbandwidth = 270 kHz
// Deviation = 47 kHz
// Datarate = 32.73 kBaud
// Modulation = (0) 2-FSK
// Manchester enable = (0) Manchester disabled
// RF Frequency = 868.299866 MHz
// Channel spacing = 199.951172 kHz
// Channel number = 0
// Optimization = -
// Sync mode = (5) 15/16 + carrier-sense above threshold
// Format of RX/TX data = (0) Normal mode, use FIFOs for RX and TX
// CRC operation = (0) CRC disabled for TX and RX
// Forward Error Correction = (0) FEC disabled
// Length configuration = (0) Fixed length packets, length configured in PKTLEN register.
// Packetlength = 255
// Preamble count = (2)  4 bytes
// Append status = 1
// Address check = (0) No address check
// FIFO autoflush = 0
// Device address = 0
// GDO0 signal selection = ( 6) Asserts when sync word has been sent / received, and de-asserts at the end of the packet
// GDO2 signal selection = (41) CHIP_RDY

#ifdef NO_PGM
#define sCFG(index)  sModeRfConfig[index]
static const uint8_t sModeRfConfig[] = {
#else
#include <avr/pgmspace.h>
#define sCFG(index)  pgm_read_byte(&sModeRfConfig[index])
static const uint8_t PROGMEM sModeRfConfig[] = {
#endif
    CC1100_SYNC1, 0x76,
    CC1100_SYNC0, 0x96,
    CC1100_MCSM1, 0x00,

    CC1100_IOCFG2, 0x06,   // IOCFG2    GDO2 output pin configuration.
    CC1100_IOCFG0, 0x00,   // IOCFG0   GDO0 output pin configuration. Refer to SmartRF® Studio User Manual for detailed pseudo register explanation.

    CC1100_FSCTRL1, 0x08,   // FSCTRL1   Frequency synthesizer control.
    CC1100_FSCTRL0, 0x00,   // FSCTRL0   Frequency synthesizer control.
    CC1100_FREQ2, 0x21,   // FREQ2     Frequency control word, high byte.
    CC1100_FREQ1, 0x65,   // FREQ1     Frequency control word, middle byte.
    CC1100_FREQ0, 0x6A,   // FREQ0     Frequency control word, low byte.
    CC1100_MDMCFG4, 0x6A,   // MDMCFG4   Modem configuration.
    CC1100_MDMCFG3, 0x4A,   // MDMCFG3   Modem configuration.
    CC1100_MDMCFG2, 0x06,   // !! 05 !! MDMCFG2   Modem configuration.
    CC1100_MDMCFG1, 0x22,   // MDMCFG1   Modem configuration.
    CC1100_MDMCFG0, 0xF8,   // MDMCFG0   Modem configuration.
    CC1100_CHANNR, 0x00,   // CHANNR    Channel number.
    CC1100_DEVIATN, 0x47,   // DEVIATN   Modem deviation setting (when FSK modulation is enabled).
    CC1100_FREND1, 0xB6,   // FREND1    Front end RX configuration.
    CC1100_FREND0, 0x10,   // FREND0    Front end RX configuration.
    CC1100_MCSM0, 0x18,   // MCSM0     Main Radio Control State Machine configuration.
    CC1100_FOCCFG, 0x2E,   // FOCCFG    Frequency Offset Compensation Configuration.
    CC1100_BSCFG, 0x6D,   // BSCFG     Bit synchronization Configuration.
    CC1100_AGCCTRL2, 0x04,   // AGCCTRL2  AGC control.
    CC1100_AGCCTRL1, 0x09,   // AGCCTRL1  AGC control.
    CC1100_AGCCTRL0, 0xB2,   // AGCCTRL0  AGC control.
    CC1100_FSCAL3, 0xEA,   // FSCAL3    Frequency synthesizer calibration.
    CC1100_FSCAL2, 0x2A,   // FSCAL2    Frequency synthesizer calibration.
    CC1100_FSCAL1, 0x00,   // FSCAL1    Frequency synthesizer calibration.
    CC1100_FSCAL0, 0x1F,   // FSCAL0    Frequency synthesizer calibration.
    CC1100_FSTEST, 0x59,   // FSTEST    Frequency synthesizer calibration.
    CC1100_TEST2, 0x81,   // TEST2     Various test settings.
    CC1100_TEST1, 0x35,   // TEST1     Various test settings.
    CC1100_TEST0, 0x09,   // TEST0     Various test settings.
    CC1100_PKTCTRL1, 0x04,   // !! 00 !! PKTCTRL1  Packet automation control.
    CC1100_PKTCTRL0, 0x00,   // PKTCTRL0  Packet automation control.
    CC1100_ADDR, 0x00,   // ADDR      Device address.
    CC1100_PKTLEN, 0xFF,   // PKTLEN    Packet length.

    CC1100_PATABLE, 0xC2,  // PATABLE
    0xff
};


#endif


/***********************************************************************************
  Copyright 2008 Texas Instruments Incorporated. All rights reserved.

  IMPORTANT: Your use of this Software is limited to those specific rights
  granted under the terms of a software license agreement between the user
  who downloaded the software, his/her employer (which must be your employer)
  and Texas Instruments Incorporated (the "License").  You may not use this
  Software unless you agree to abide by the terms of the License. The License
  limits your use, and you acknowledge, that the Software may not be modified,
  copied or distributed unless embedded on a Texas Instruments microcontroller
  or used solely and exclusively in conjunction with a Texas Instruments radio
  frequency transceiver, which is integrated into your product.  Other than for
  the foregoing purpose, you may not use, reproduce, copy, prepare derivative
  works of, modify, distribute, perform, display or sell this Software and/or
  its documentation for any purpose.

  YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE
  PROVIDED “AS IS” WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
  INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY, TITLE,
  NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL
  TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
  NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER
  LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
  INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE
  OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT
  OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
  (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.

  Should you have any questions regarding your right to use this Software,
  contact Texas Instruments Incorporated at www.TI.com.
***********************************************************************************/

