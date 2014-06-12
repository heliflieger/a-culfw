/***********************************************************************************
    Filename: mbus_paket.h
***********************************************************************************/

#ifndef MBUS_PACKET_H
#define MBUS_PACKET_H

//----------------------------------------------------------------------------------
//  Constants 
//----------------------------------------------------------------------------------

#define PACKET_C_FIELD  0x44
#define MAN_CODE        0x0CAE
#define MAN_NUMBER      0x12345678
#define MAN_ID          0x01
#define MAN_VER         0x07
#define PACKET_CI_FIELD 0x78

#define PACKET_OK              0
#define PACKET_CODING_ERROR    1
#define PACKET_CRC_ERROR       2

#define WMBUS_SMODE            1
#define WMBUS_TMODE            2

//----------------------------------------------------------------------------------
// Functions
//----------------------------------------------------------------------------------

void encodeTXPacket(uint8* pPacket, uint8* pData, uint8 dataSize);

uint16 packetSize(uint8 lField);
uint16 byteSize(uint8 Smode, uint8 transmit, uint16 packetSize);

void   encodeTXBytesTmode(uint8* pByte, uint8* pPacket, uint16 packetSize);
uint16 decodeRXBytesTmode(uint8* pByte, uint8* pPacket, uint16 packetSize);

void   encodeTXBytesSmode(uint8* pByte, uint8* pPacket, uint16 packetSize);
uint16 decodeRXBytesSmode(uint8* pByte, uint8* pPacket, uint16 packetSize);


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

