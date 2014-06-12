/******************************************************************************
    Filename: mbus_packet.c
******************************************************************************/

#include "mbus_defs.h"
#include "mbus_packet.h"
#include "manchester.h"
#include "3outof6.h"
#include "crc.h"


//----------------------------------------------------------------------------------
//  Functions
//----------------------------------------------------------------------------------

//----------------------------------------------------------------------------------
//  uint16 packetSize (uint8 lField)
//
//  DESCRIPTION:
//    Returns the number of bytes in a Wireless MBUS packet from
//    the L-field. Note that the L-field excludes the L-field and the 
//    CRC fields
//
//  ARGUMENTS:  
//    uint8 lField  - The L-field value in a Wireless MBUS packet
//
//  RETURNS
//    uint16        - The number of bytes in a wireless MBUS packet 
//----------------------------------------------------------------------------------

uint16 packetSize (uint8 lField)
{
  uint16 nrBytes;
  uint8  nrBlocks;
  
  // The 2 first blocks contains 25 bytes when excluding CRC and the L-field
  // The other blocks contains 16 bytes when excluding the CRC-fields
  // Less than 26 (15 + 10) 
  if ( lField < 26 ) 
    nrBlocks = 2;
  else 
    nrBlocks = (((lField - 26) / 16) + 3);
  
  // Add all extra fields, excluding the CRC fields
  nrBytes = lField + 1;

  // Add the CRC fields, each block is contains 2 CRC bytes
  nrBytes += (2 * nrBlocks);
      
  return (nrBytes);
}




//----------------------------------------------------------------------------------
//  uint16 byteSize (uint8 Smode, uint8 transmit, uint16 packetSize)
//
//  DESCRIPTION:
//    Returns the total number of encoded bytes to receive or transmit, given the 
//    total number of bytes in a Wireless MBUS packet. 
//    In receive mode the postamble sequence and synchronization word is excluded
//    from the calculation.
//
//  ARGUMENTS:  
//    uint8   Smode       - S-mode or T-mode
//    uint8   transmit    - Transmit or receive
//    uint16  packetSize  - Total number of bytes in the wireless MBUS packet
//
//  RETURNS
//    uint16  - The number of bytes of the encoded WMBUS packet
//----------------------------------------------------------------------------------
uint16 byteSize (uint8 Smode, uint8 transmit, uint16 packetSize)
{
  uint16 tmodeVar;
  
  // S-mode, data is Manchester coded
  if (Smode)
  {
    // Transmit mode
    // 1 byte for postamble and 1 byte synchronization word
    if (transmit)
      return (2*packetSize + 2);
    
    // Receive mode
    else
      return (2*packetSize);
  }
  
  // T-mode
  // Data is 3 out of 6 coded 
  else
  { 
    tmodeVar = (3*packetSize) / 2;
    
    // Transmit mode
    // + 1 byte for the postamble sequence
    if (transmit)
       return (tmodeVar + 1);

    // Receive mode
    // If packetsize is a odd number 1 extra byte   
    // that includes the 4-postamble sequence must be
    // read.    
    else
    {
      if (packetSize % 2)
        return (tmodeVar + 1);
      else 
        return (tmodeVar);
    }
  }   
}



//----------------------------------------------------------------------------------
//  void encodeTXPacket(uint8* pPacket, uint8* pData, uint8 dataSize)
//
//  DESCRIPTION:
//    Encode n data bytes into a Wireless MBUS packet format. 
//    The function will add all the control field, calculates and inserts the 
//    the CRC fields
//
//   ARGUMENTS:  
//    uint8 *pPacket    - Pointer to the WMBUS packet byte table
//    uint8 *pData      - Pointer to user data byte table
//    uint8  dataSize   - Number of user data bytes. Max size is 245
//----------------------------------------------------------------------------------
void encodeTXPacket(uint8* pPacket, uint8* pData, uint8 dataSize)
{
    
  uint8 loopCnt;
  uint8 dataRemaining;
  uint8 dataEncoded;
  uint16 crc;

  dataRemaining = dataSize;
  dataEncoded  = 0;
  crc = 0;
  
  // **** Block 1 *****

   // - L-Field - 
  // The length field excludes all CRC-fields and the L-field,
  // e.g. L = dataSize + 10
  *pPacket = dataSize + 10;
  crc = crcCalc(crc,*pPacket);
  pPacket++;

  // - C-Field - 
  *(pPacket) = PACKET_C_FIELD;
  crc = crcCalc(crc,*pPacket);
  pPacket++;
  
  // - M-Field - 
  *(pPacket) = (uint8)MAN_CODE;
  crc = crcCalc(crc,*pPacket);
  pPacket++;
  *(pPacket) = (uint8)(MAN_CODE >> 8);
  crc = crcCalc(crc,*pPacket);
  pPacket++;
  
  // - A-Field - 
  *(pPacket) = (uint8)MAN_NUMBER;
  crc = crcCalc(crc,*pPacket);
  pPacket++;
  *(pPacket) = (uint8)(MAN_NUMBER >> 8);
  crc = crcCalc(crc,*pPacket);
  pPacket++;
  *(pPacket) = (uint8)(MAN_NUMBER >> 16);
  crc = crcCalc(crc,*pPacket);
  pPacket++;
  *(pPacket) = (uint8)(MAN_NUMBER >> 24);
  crc = crcCalc(crc,*pPacket);
  pPacket++;
  *(pPacket) = (uint8)(MAN_ID);
  crc = crcCalc(crc,*pPacket);
  pPacket++;
  *(pPacket) = (uint8)(MAN_VER);
  crc = crcCalc(crc,*pPacket);
  pPacket++;
 
  // - CRC -
  *(pPacket) = HI_UINT16(~crc);
  pPacket++;
  *(pPacket) = LO_UINT16(~crc);
  pPacket++;
  crc = 0;


  // **** Block 2 *****    
  
  // - CI-Field - 
  *(pPacket) = PACKET_CI_FIELD;
  crc = crcCalc(crc,*pPacket);
  pPacket++;
   
  // Check if last Block
  if (dataRemaining < 16)
  {
    // Data Fields
    for ( loopCnt = 0; loopCnt < dataRemaining ; loopCnt = loopCnt + 1)
    {
      *(pPacket) = pData[dataEncoded];
      crc = crcCalc(crc,*pPacket);
      pPacket++;
      dataEncoded++;
    }
    
    // CRC  
     *(pPacket) = HI_UINT16(~crc);
    pPacket++;
    *(pPacket) = LO_UINT16(~crc);
    pPacket++;
    crc = 0;
    dataRemaining = 0;
  }
  
  else 
  {
    // Data Fields
    for ( loopCnt = 0; loopCnt < 15; loopCnt = loopCnt + 1)
    {
      *(pPacket) = pData[dataEncoded];
      crc = crcCalc(crc,*pPacket);
      pPacket++;
      dataEncoded++;
    }
   
    *(pPacket) = HI_UINT16(~crc);
    pPacket++;
    *(pPacket) = LO_UINT16(~crc);
    pPacket++;
    crc = 0;
    dataRemaining -= 15;
  }
  

  // **** Block n *****    
  while (dataRemaining)
  {
   // Check if last Block
    if (dataRemaining < 17)
    {
      // Data Fields
      for ( loopCnt = 0; loopCnt < dataRemaining ; loopCnt = loopCnt + 1)
      {
        *(pPacket) = pData[dataEncoded];
         crc = crcCalc(crc,*pPacket);
         pPacket++;
         dataEncoded++;
      }
      
    // CRC
    *(pPacket) = HI_UINT16(~crc);
    pPacket++;
    *(pPacket) = LO_UINT16(~crc);
    pPacket++;
    crc = 0;
    dataRemaining = 0;
    }
  
    else 
    {
      // Data Fields
      for ( loopCnt = 0; loopCnt < 16; loopCnt = loopCnt + 1)
      {
        *(pPacket) = pData[dataEncoded];
        crc = crcCalc(crc,*pPacket);
        pPacket++;
        dataEncoded++;
      }
    
    // CRC
    *(pPacket) = HI_UINT16(~crc);
    pPacket++;
    *(pPacket) = LO_UINT16(~crc);
    pPacket++;
    crc = 0;
    dataRemaining -= 16;
    }      
  } 
} 



      
//----------------------------------------------------------------------------------
//  void encodeTXBytesSmode(uint8* pByte, uint8* pPacket, uint16 packetSize)
//
//  DESCRIPTION:
//    Encodes a wireless MBUS packet into a SMODE packet. This includes
//    - Add Least Significant Byte of synchronization word to the TX array
//    - Manchester encode the Wireless MBUS packet.
//    - Add postamble sequence to the TX array
//
//   ARGUMENTS:  
//    uint8* pByte        - Pointer to SMODE packet to transmit
//    uint8* pPacket      - Pointer to Wireless MBUS packet
//    uint16 packetSize   - Total Size of the uncoded Wireless MBUS packet
//
//   RETURNS
//    uint16              - Total size of bytes to transmit
//----------------------------------------------------------------------------------

void encodeTXBytesSmode(uint8* pByte, uint8* pPacket, uint16 packetSize)
{
  uint16 bytesEncoded;

  bytesEncoded  = 0;

  // Last byte of synchronization word  
  (*(pByte)) = 0x96;
  pByte++;
  
  // Manchester encode packet
  while (bytesEncoded < packetSize)
  {
    manchEncode((pPacket + bytesEncoded), (pByte + 2*bytesEncoded));
    bytesEncoded++;
  }

  // Append the postamble sequence
  (*(pByte + 2*bytesEncoded)) = 0x55;

}


      
//----------------------------------------------------------------------------------
//  void encodeTXBytesTmode(uint8* pByte, uint8* pPacket, uint16 packetSize)
//
//  DESCRIPTION:
//    Encodes a wireless MBUS packet into a TMODE packet. This includes
//    - 3 out of 6 encode the Wireless MBUS packet.
//    - Append postamble sequence to the TX array
//
//   ARGUMENTS:  
//    uint8* pByte        - Pointer to TMODE packet
//    uint8* pPacket      - Pointer to Wireless MBUS packet
//    uint16 packetSize   - Total size of the Wireless MBUS packet
//----------------------------------------------------------------------------------
void encodeTXBytesTmode(uint8* pByte, uint8* pPacket, uint16 packetSize)
{
  uint16 bytesRemaining;

  bytesRemaining = packetSize;
  
  // 3 our of 6 encode packet
  while (bytesRemaining)
  {
    // If 1 byte left to encode, include 
    // Postamble in "3 out of 6" encoding routine
    if (bytesRemaining == 1)
    {
      encode3outof6(pPacket, pByte, 1);
      bytesRemaining -= 1;
    }

    // Else if 2 byte left to encode, append Postamble
    else if (bytesRemaining == 2)
    {
      encode3outof6(pPacket, pByte, 0);
      
      // Append postamble
      pByte += 3;
      *pByte = 0x55;
      bytesRemaining -= 2; 
    }
    else
    {
      encode3outof6(pPacket, pByte, 0);
      pByte += 3;
      pPacket += 2;
      bytesRemaining -= 2; 
    }
  }  
}


//----------------------------------------------------------------------------------
//  uint16 decodeRXBytesSmode(uint8* pByte, uint8* pPacket, uint16 packetSize)
//
//  DESCRIPTION:
//    Decode a SMODE packet into a Wireless MBUS packet. Checks for 3 out of 6
//    decoding errors and CRC errors.
//
//   ARGUMENTS:  
//    uint8 *pByte        - Pointer to SMBUS packet
//    uint8 *pPacket      - Pointer to Wireless MBUS packet
//    uint16 packetSize   - Total Size of the Wireless MBUS packet
//
//    RETURNS:
//    PACKET_OK              0
//    PACKET_CODING_ERROR    1
//    PACKET_CRC_ERROR       2
//----------------------------------------------------------------------------------
uint16 decodeRXBytesSmode(uint8* pByte, uint8* pPacket, uint16 packetSize)
{
  uint16 bytesRemaining;
  uint16 bytesEncoded;
  uint16 decodingStatus;
  uint16 crc;             // Current CRC
  uint16 crcField1;       // Current byte is CRC high byte
  uint16 crcField0;       // Current byte is CRC low byte
  
  bytesRemaining = packetSize;
  bytesEncoded = 0;
  crcField1 = 0;
  crcField0 = 0;
  crc       = 0;

  // Decode packet
  while (bytesRemaining)
  {
    decodingStatus = manchDecode(pByte, pPacket);
    
    // Check for valid Manchester decoding
    if ( decodingStatus != MAN_DECODING_OK)
      return (PACKET_CODING_ERROR);


    // Check if current field is CRC field number 1, e.g.
    // - Field 10 + 18*n
    // - Less than 2 bytes
    if (bytesRemaining == 2)
      crcField1 = 1;

    else if ( bytesEncoded > 9 )
      crcField1 = !((bytesEncoded - 10) % 18);

    
    // If CRC field number 0, check the low byte of the CRC
    if (crcField0)
    {        
     if (LO_UINT16(~crc) != *pPacket )
      return (PACKET_CRC_ERROR);

     crcField0 = 0;        
     crc = 0;
    }      

    // If CRC field number 1, check the high byte of the CRC
    else if (crcField1)
    {
      if (HI_UINT16(~crc) != *pPacket )
        return (PACKET_CRC_ERROR);
     
        // Next field is CRC field 1 
       crcField0 = 1;
       crcField1 = 0;
    }
        
    // If not a CRC field, increment CRC calculation
    else    
      crc = crcCalc(crc, *pPacket);


    bytesRemaining--;
    bytesEncoded++;      
    pByte += 2;
    pPacket++;

  }
  return (PACKET_OK);
}



//----------------------------------------------------------------------------------
//  uint16 decodeRXBytesTmode(uint8* pByte, uint8* pPacket, uint16 packetSize)
//
//  DESCRIPTION:
//    Decode a TMODE packet into a Wireless MBUS packet. Checks for 3 out of 6
//    decoding errors and CRC errors.
//
//   ARGUMENTS:  
//    uint8 *pByte        - Pointer to TMBUS packet
//    uint8 *pPacket      - Pointer to Wireless MBUS packet
//    uint16 packetSize   - Total Size of the Wireless MBUS packet
//
//   RETURNS:
//    PACKET_OK              0
//    PACKET_CODING_ERROR    1
//    PACKET_CRC_ERROR       2
//----------------------------------------------------------------------------------
uint16 decodeRXBytesTmode(uint8* pByte, uint8* pPacket, uint16 packetSize)
{
    
  uint16 bytesRemaining;
  uint16 bytesEncoded;
  uint16 decodingStatus;
  uint16 crc;               // Current CRC value
  uint16 crcField;          // Current fields are a CRC field

    
  bytesRemaining = packetSize;
  bytesEncoded   = 0;
  crcField       = 0;
  crc            = 0;
      
  // Decode packet      
  while (bytesRemaining)
  {
    // If last byte
    if (bytesRemaining == 1)
    {
      decodingStatus = decode3outof6(pByte, pPacket, 1);
      
      // Check for valid 3 out of 6 decoding
      if ( decodingStatus != DECODING_3OUTOF6_OK)
        return (PACKET_CODING_ERROR);
      
      bytesRemaining  -= 1;
      bytesEncoded    += 1;
      
      // The last byte the low byte of the CRC field
     if (LO_UINT16(~crc) != *(pPacket ))
        return (PACKET_CRC_ERROR);
    }
         
    else
    {
      
      decodingStatus = decode3outof6(pByte, pPacket, 0);
      
      // Check for valid 3 out of 6 decoding
      if ( decodingStatus != DECODING_3OUTOF6_OK)
        return (PACKET_CODING_ERROR);
        
      bytesRemaining -= 2; 
      bytesEncoded  += 2;
      
      
      // Check if current field is CRC fields
      // - Field 10 + 18*n
      // - Less than 2 bytes
      if (bytesRemaining == 0)
        crcField = 1;
      else if ( bytesEncoded > 10 )
        crcField = !((bytesEncoded - 12) % 18);
      
      // Check CRC field
      if (crcField)
      {        
       if (LO_UINT16(~crc) != *(pPacket + 1 ))
        return (PACKET_CRC_ERROR);
       if (HI_UINT16(~crc) != *pPacket)
          return (PACKET_CRC_ERROR);
       
       crcField = 0;        
       crc = 0;
      }
      
      // If 1 bytes left, the field is the high byte of the CRC
      else if (bytesRemaining == 1)
      {
        crc = crcCalc(crc, *(pPacket));
        // The packet byte is a CRC-field
       if (HI_UINT16(~crc) != *(pPacket + 1))
        return (PACKET_CRC_ERROR);
      }

      // Perform CRC calculation           
      else
       {
        crc = crcCalc(crc, *(pPacket));
        crc = crcCalc(crc, *(pPacket + 1));
       }
   
      pByte += 3;
      pPacket += 2;
      
    }
  }
  
  return (PACKET_OK);
}


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


