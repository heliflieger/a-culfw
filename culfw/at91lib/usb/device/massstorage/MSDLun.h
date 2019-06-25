/* ----------------------------------------------------------------------------
 *         ATMEL Microcontroller Software Support 
 * ----------------------------------------------------------------------------
 * Copyright (c) 2008, Atmel Corporation
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the disclaimer below.
 *
 * Atmel's name may not be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * DISCLAIMER: THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * ----------------------------------------------------------------------------
 */

//------------------------------------------------------------------------------
/// \unit
/// !Purpose
/// 
/// Logical Unit Number (LUN) used by the Mass Storage driver and the SCSI
/// protocol. Represents a logical hard-drive.
/// 
/// !Usage
/// -# Initialize Memory related pins (see pio & board.h).
/// -# Initialize a Media instance for the LUN (see memories).
/// -# Initlalize the LUN with LUN_Init, and link to the initialized Media.
/// -# To read data from the LUN linked media, uses LUN_Read.
/// -# To write data to the LUN linked media, uses LUN_Write.
//------------------------------------------------------------------------------

#ifndef MSDLUN_H
#define MSDLUN_H

//------------------------------------------------------------------------------
//         Headers
//------------------------------------------------------------------------------

#include "SBC.h"
#include <memories/Media.h>
#include <usb/device/core/USBD.h>

//------------------------------------------------------------------------------
//      Definitions
//------------------------------------------------------------------------------

#define LUN_STATUS_SUCCESS          0x00
#define LUN_STATUS_ERROR            0x02

//------------------------------------------------------------------------------
//      Structures
//------------------------------------------------------------------------------

/// LUN structure
typedef struct {

    /// Pointer to a SBCInquiryData instance.
    SBCInquiryData        *inquiryData;
    /// Buffer for USB transfer, must be assigned.
    unsigned char         *readWriteBuffer;
    /// Data for the RequestSense command.
    SBCRequestSenseData   requestSenseData;
    /// Data for the ReadCapacity command.
    SBCReadCapacity10Data readCapacityData;
    /// Pointer to Media instance for the LUN.
    Media                 *media;
    /// The start position of the media allocated to the LUN.
    unsigned int          baseAddress;
    /// The size of the media allocated to the LUN.
    unsigned int          size;
    /// Sector size of the media
    unsigned int          blockSize;

} MSDLun;

//------------------------------------------------------------------------------
//      Exported functions
//------------------------------------------------------------------------------
extern void LUN_Init(MSDLun         *lun,
                     Media       *media,
                     unsigned char *buffer,
                     unsigned int  baseAddress,
                     unsigned int  size,
                     unsigned int  blockSize);

extern unsigned char LUN_Write(MSDLun *lun,
                               unsigned int blockAddress,
                               void         *data,
                               unsigned int length,
                               TransferCallback   callback,
                               void         *argument);

extern unsigned char LUN_Read(MSDLun        *lun,
                              unsigned int blockAddress,
                              void         *data,
                              unsigned int length,
                              TransferCallback   callback,
                              void         *argument);

#endif //#ifndef MSDLUN_H

