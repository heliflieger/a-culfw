/******************************************************************************
    File name: manchester.c
******************************************************************************/

#include "mbus_defs.h"
#include "manchester.h"

//----------------------------------------------------------------------------
// Variables
//----------------------------------------------------------------------------

// Table for encoding 4-bit data into a 8-bit Manchester encoding.
static uint8 manchEncodeTab[16] = {0xAA,  // 0x0 Manchester encoded 
                                   0xA9,  // 0x1 Manchester encoded 
                                   0xA6,  // 0x2 Manchester encoded 
                                   0xA5,  // 0x3 Manchester encoded 
                                   0x9A,  // 0x4 Manchester encoded 
                                   0x99,  // 0x5 Manchester encoded 
                                   0x96,  // 0x6 Manchester encoded 
                                   0x95,  // 0x7 Manchester encoded                                                                                    
                                   0x6A,  // 0x8 Manchester encoded   
                                   0x69,  // 0x9 Manchester encoded 
                                   0x66,  // 0xA Manchester encoded 
                                   0x65,  // 0xB Manchester encoded                  
                                   0x5A,  // 0xC Manchester encoded  
                                   0x59,  // 0xD Manchester encoded 
                                   0x56,  // 0xE Manchester encoded 
                                   0x55}; // 0xF Manchester encoded

// Table for decoding 4-bit Manchester encoded data into 2-bit 
// data. 0xFF indicates invalid Manchester encoding
static uint8 manchDecodeTab[16] = {0xFF, //  Manchester encoded 0x0 decoded
                                   0xFF, //  Manchester encoded 0x1 decoded
                                   0xFF, //  Manchester encoded 0x2 decoded
                                   0xFF, //  Manchester encoded 0x3 decoded
                                   0xFF, //  Manchester encoded 0x4 decoded
                                   0x03, //  Manchester encoded 0x5 decoded
                                   0x02, //  Manchester encoded 0x6 decoded
                                   0xFF, //  Manchester encoded 0x7 decoded                                                                                   
                                   0xFF, //  Manchester encoded 0x8 decoded  
                                   0x01, //  Manchester encoded 0x9 decoded
                                   0x00, //  Manchester encoded 0xA decoded
                                   0xFF, //  Manchester encoded 0xB decoded
                                   0xFF, //  Manchester encoded 0xC decoded 
                                   0xFF, //  Manchester encoded 0xD decoded
                                   0xFF, //  Manchester encoded 0xE decoded
                                   0xFF}; //  Manchester encoded 0xF decoded
                            

                            
//----------------------------------------------------------------------------
// Functions
//----------------------------------------------------------------------------
                                                      
//----------------------------------------------------------------------------
//  void manchEncode(uint8 *uncodedData, uint8 *encodedData)
//                                                          
//  DESCRIPTION:                                            
//    Perfoms Manchester coding on 8-bit data
//
//  ARGUMENTS:  
//        uint8 *uncodedData    - Pointer to data
//        uint8 *encodedData    - Pointer to store the encoded data
//----------------------------------------------------------------------------

void manchEncode(uint8 *uncodedData, uint8 *encodedData)
{
  uint8  data0, data1;
  
  // - Shift to get 4-bit data values
  data1 = (((*uncodedData) >> 4) & 0x0F);
  data0 = ((*uncodedData) & 0x0F);


  // - Perform Manchester encoding - 
  *encodedData       = (manchEncodeTab[data1]);
  *(encodedData + 1) = manchEncodeTab[data0];
  
}



//----------------------------------------------------------------------------
//  uint8 manchDecode(uint8 *encodedData, uint8 *decodedData)
//
//  DESCRIPTION:
//    Perfoms Manchester decoding on 16-bit data
//
//  ARGUMENTS:
//        uint8 *encodedData    - Pointer to encoded data
//        uint8 *decodedData    - Pointer to store the decoded data
//   
//  RETURNS
//        MAN_DECODING_OK      0
//        MAN_DECODING_ERROR   1
//----------------------------------------------------------------------------

uint8 manchDecode(uint8 *encodedData, uint8 *decodedData)
{
  uint8 data0, data1, data2, data3;

  // - Shift to get 4 bit data and decode
  data3 = ((*encodedData >> 4) & 0x0F);
  data2 = ( *encodedData       & 0x0F);
  data1 = ((*(encodedData + 1) >> 4) & 0x0F);
  data0 = ((*(encodedData + 1))      & 0x0F);

  // Check for invalid Manchester encoding
  if ( (manchDecodeTab[data3] == 0xFF ) | (manchDecodeTab[data2] == 0xFF ) |
     (manchDecodeTab[data1] == 0xFF ) | (manchDecodeTab[data0] == 0xFF ) )
  {
    return(MAN_DECODING_ERROR);
  }

  // Shift result into a byte
  *decodedData = (manchDecodeTab[data3] << 6) | (manchDecodeTab[data2] << 4) |
                 (manchDecodeTab[data1] << 2) |  manchDecodeTab[data0];

  return(MAN_DECODING_OK);
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

