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
//         Headers
//------------------------------------------------------------------------------


#include <usb/device/massstorage/MSDLun.h>
#include <utility/trace.h>
#include <usb/device/core/USBD.h>
#include "flash/flashd.h"

//------------------------------------------------------------------------------
//         Internal variables
//------------------------------------------------------------------------------

typedef struct {
    uint8_t JumpInstruction[3];
    uint8_t OEMInfo[8];
    uint16_t SectorSize;
    uint8_t SectorsPerCluster;
    uint16_t ReservedSectors;
    uint8_t FATCopies;
    uint16_t RootDirectoryEntries;
    uint16_t TotalSectors16;
    uint8_t MediaDescriptor;
    uint16_t SectorsPerFAT;
    uint16_t SectorsPerTrack;
    uint16_t Heads;
    uint32_t HiddenSectors;
    uint32_t TotalSectors32;
    uint8_t PhysicalDriveNum;
    uint8_t Reserved;
    uint8_t ExtendedBootSig;
    uint32_t VolumeSerialNumber;
    uint8_t VolumeLabel[11];
    uint8_t FilesystemIdentifier[8];
} __attribute__((packed)) FAT_BootBlock;

typedef struct {
    char name[8];
    char ext[3];
    uint8_t attrs;
    uint8_t reserved;
    uint8_t createTimeFine;
    uint16_t createTime;
    uint16_t createDate;
    uint16_t lastAccessDate;
    uint16_t highStartCluster;
    uint16_t updateTime;
    uint16_t updateDate;
    uint16_t startCluster;
    uint32_t size;
} __attribute__((packed)) DirEntry;

#define NUM_FAT_BLOCKS  8000

#define RESERVED_SECTORS 1
#define ROOT_DIR_SECTORS 4
#define SECTORS_PER_FAT ((NUM_FAT_BLOCKS * 2 + 511) / 512)

#define START_FAT0 RESERVED_SECTORS
#define START_FAT1 (START_FAT0 + SECTORS_PER_FAT)
#define START_ROOTDIR (START_FAT1 + SECTORS_PER_FAT)
#define START_CLUSTERS (START_ROOTDIR + ROOT_DIR_SECTORS)

static const FAT_BootBlock BootBlock = {
    .JumpInstruction = {0xeb, 0x3c, 0x90},
    .OEMInfo = "UF2 UF2 ",
    .SectorSize = 512,
    .SectorsPerCluster = 1,
    .ReservedSectors = RESERVED_SECTORS,
    .FATCopies = 2,
    .RootDirectoryEntries = (ROOT_DIR_SECTORS * 512 / 32),
    .TotalSectors16 = NUM_FAT_BLOCKS - 2,
    .MediaDescriptor = 0xF8,
    .SectorsPerFAT = SECTORS_PER_FAT,
    .SectorsPerTrack = 1,
    .Heads = 1,
    .ExtendedBootSig = 0x29,
    .VolumeSerialNumber = 0x00420042,
    .VolumeLabel = "CUBE",
    .FilesystemIdentifier = "FAT16   ",
};

/// Inquiry data to return to the host for the Lun.
static SBCInquiryData inquiryData = {

    SBC_DIRECT_ACCESS_BLOCK_DEVICE,  // Direct-access block device
    SBC_PERIPHERAL_DEVICE_CONNECTED, // Peripheral device is connected
    0x00,                            // Reserved bits
    0x01,                            // Media is removable
    SBC_SPC_VERSION_4,               // SPC-4 supported
    0x2,                             // Response data format, must be 0x2
    0,                           // Hierarchical addressing not supported
    0,                           // ACA not supported
    0x0,                             // Obsolete bits
    sizeof(SBCInquiryData) - 5,  // Additional length
    0,                           // No embedded SCC
    0,                           // No access control coordinator
    SBC_TPGS_NONE,                   // No target port support group
    0,                           // Third-party copy not supported
    0x0,                             // Reserved bits
    0,                           // Protection information not supported
    0x0,                             // Obsolete bit
    0,                           // No embedded enclosure service component
    0x0,                             // ???
    0,                           // Device is not multi-port
    0x0,                             // Obsolete bits
    0x0,                             // Unused feature
    0x0,                             // Unused features
    0,                           // Task management model not supported
    0x0,                             // ???
    {'A','T','M','E','L',' ',' ',' '},
    {'M','a','s','s',' ','S','t','o','r','a','g','e',' ','M','S','D'},
    {'0','.','0','1'},
    {'M','a','s','s',' ','S','t','o','r','a','g','e',' ','E','x','a','m','p','l','e'},
    0x00,                            // Unused features
    0x00,                            // Reserved bits
    {SBC_VERSION_DESCRIPTOR_SBC_3},    // SBC-3 compliant device
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0} // Reserved
};

unsigned int writeaddress=TARGETSTART;

//------------------------------------------------------------------------------
//      Internal Functions
//------------------------------------------------------------------------------

int read_block(uint32_t block_no, uint8_t *data) {
    for(uint16_t x=0; x<512;x++) {
      data[x] = 0;
    }

    uint32_t sectionIdx = block_no;

    if (block_no == 0) {
        memcpy(data, &BootBlock, sizeof(BootBlock));
        data[510] = 0x55;
        data[511] = 0xaa;
    } else if (block_no < START_ROOTDIR) {
        sectionIdx -= START_FAT0;;
        if (sectionIdx >= SECTORS_PER_FAT)
            sectionIdx -= SECTORS_PER_FAT;
        if (sectionIdx == 0) {
            data[0] = 0xf0;
            for (int i = 1; i < 4; ++i) {
                data[i] = 0xff;
            }
        }

    } else if (block_no < START_CLUSTERS) {
        sectionIdx -= START_ROOTDIR;

    } else {
        sectionIdx -= START_CLUSTERS;

    }

    return 0;
}

//------------------------------------------------------------------------------
//         Exported functions
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//! \brief  Initializes a LUN instance.
//! \param  lun         Pointer to the MSDLun instance to initialize
//! \param  media       Media on which the LUN is constructed
//! \param  buffer      Pointer to a buffer used for read/write operation and
//!                      which must be blockSize bytes long.
//! \param  baseAddress Base address of the LUN on the media
//! \param  size        Total size of the LUN in bytes
//! \param  blockSize   Length of one block of the LUN
//------------------------------------------------------------------------------
void LUN_Init(MSDLun         *lun,
              Media       *media,
              unsigned char *buffer,
              unsigned int  baseAddress,
              unsigned int  size,
              unsigned int  blockSize)
{
    unsigned int logicalBlockAddress = (size / blockSize) - 1;
    TRACE_INFO("LUN init\n\r");

    // Initialize LUN
    lun->media = media;
    lun->baseAddress = baseAddress;
    lun->size = size;
    lun->blockSize = blockSize;
    lun->readWriteBuffer = buffer;

    // Initialize request sense data
    lun->requestSenseData.bResponseCode = SBC_SENSE_DATA_FIXED_CURRENT;
    lun->requestSenseData.isValid = 1;
    lun->requestSenseData.bObsolete1 = 0;
    lun->requestSenseData.bSenseKey = SBC_SENSE_KEY_NO_SENSE;
    lun->requestSenseData.bReserved1 = 0;
    lun->requestSenseData.isILI = 0;
    lun->requestSenseData.isEOM = 0;
    lun->requestSenseData.isFilemark = 0;
    lun->requestSenseData.pInformation[0] = 0;
    lun->requestSenseData.pInformation[1] = 0;
    lun->requestSenseData.pInformation[2] = 0;
    lun->requestSenseData.pInformation[3] = 0;
    lun->requestSenseData.bAdditionalSenseLength
        = sizeof(SBCRequestSenseData) - 8;
    lun->requestSenseData.bAdditionalSenseCode = 0;
    lun->requestSenseData.bAdditionalSenseCodeQualifier = 0;
    lun->requestSenseData.bFieldReplaceableUnitCode = 0;
    lun->requestSenseData.bSenseKeySpecific = 0;
    lun->requestSenseData.pSenseKeySpecific[0] = 0;
    lun->requestSenseData.pSenseKeySpecific[0] = 0;
    lun->requestSenseData.isSKSV = 0;

    // Initialize inquiry data
    lun->inquiryData = &inquiryData;

    // Initialize read capacity data
    STORE_DWORDB(logicalBlockAddress,
                 lun->readCapacityData.pLogicalBlockAddress);
    STORE_DWORDB(blockSize, lun->readCapacityData.pLogicalBlockLength);
}

extern volatile int timeout;

//------------------------------------------------------------------------------
//! \brief  Writes data on the a LUN starting at the specified block address.
//! \param  pLUN          Pointer to a MSDLun instance
//! \param  blockAddress First block address to write
//! \param  data         Pointer to the data to write
//! \param  length       Number of blocks to write
//! \param  callback     Optional callback to invoke when the write finishes
//! \return Operation result code
//------------------------------------------------------------------------------
__attribute__((optimize("Og"))) unsigned char LUN_Write(MSDLun        *lun,
                        unsigned int blockAddress,
                        void         *data,
                        unsigned int length,
                        TransferCallback   callback,
                        void         *argument)
{
    unsigned int  address;
    unsigned char status;
    static char binStart=0;

    TRACE_INFO_WP("LUNWrite(%u) ", blockAddress);

    // Check that the data is not too big
    if ((length * lun->blockSize)
        > (lun->size - lun->blockSize * blockAddress)) {

        TRACE_WARNING("LUN_Write: Data too big\n\r");
        status = USBD_STATUS_ABORTED;
    }
    else {

        TRACE_WARNING_WP("LUNWrite:%u len:%u \n\r", blockAddress, length);

        unsigned long i;
        unsigned char* fl;
        unsigned char result;
        unsigned char* writebuffer = (unsigned char*)data;


        if (!binStart && (((writebuffer[2]+(writebuffer[3]<<8))) == 0xE59F)) {
          binStart = 1;
          TRACE_WARNING_WP("Bin found \n\r");
        }

        if((blockAddress>0x44) && binStart) {
          result=FLASHD_Write(writeaddress,writebuffer,lun->blockSize);
          TRACE_WARNING_WP("MSC flash: ");
          fl=writeaddress;
          for (i=0;i<lun->blockSize;i++) {
            if (fl[i] != writebuffer[i]) break;
          }
          TRACE_WARNING_WP("%08X: %X : %X\n\r",writeaddress,result,i);
          writeaddress+=lun->blockSize;
          timeout = 8;
        }

        status = MED_STATUS_SUCCESS;

        if (callback != 0) {
           callback(argument, MED_STATUS_SUCCESS, 0, 0);
        }

        // Check operation result code
        if (status == MED_STATUS_SUCCESS) {

            status = USBD_STATUS_SUCCESS;
        }
        else {

            TRACE_WARNING("LUN_Write: Cannot write media\n\r");
            status = USBD_STATUS_ABORTED;
        }
    }

    return status;
}

//------------------------------------------------------------------------------
//! \brief  Reads data from a LUN, starting at the specified block address.
//! \param  pLUN          Pointer to a MSDLun instance
//! \param  blockAddress First block address to read
//! \param  data         Pointer to a data buffer in which to store the data
//! \param  length       Number of blocks to read
//! \param  callback     Optional callback to invoke when the read finishes
//! \return Operation result code
//------------------------------------------------------------------------------
unsigned char LUN_Read(MSDLun        *lun,
                       unsigned int blockAddress,
                       void         *data,
                       unsigned int length,
                       TransferCallback   callback,
                       void         *argument)
{
    unsigned int address;
    unsigned char status;

    // Check that the data is not too big
    if ((length * lun->blockSize)
        > (lun->size - lun->blockSize * blockAddress)) {

        TRACE_WARNING("LUN_Read: Data too big\n\r");
        status = USBD_STATUS_ABORTED;
    }
    else {

        TRACE_INFO_WP("LUNRead(%u) ", blockAddress);

        TRACE_WARNING_WP("LUNRead:%u len:%u \n\r", blockAddress, length);

        // Compute read start address
        //address = lun->media->baseAddress
        //           + lun->baseAddress
        //           + blockAddress * lun->blockSize;

        // Start write operation
        status = read_block(blockAddress,data);
        //status = MED_Read(lun->media,
        //                  address,
        //                 data,
        //                  length * lun->blockSize,
        //                  (MediaCallback) callback,
        //                  argument);

        status = MED_STATUS_SUCCESS;

        // Invoke callback
        if (callback != 0) {
            callback(argument, MED_STATUS_SUCCESS, 0, 0);
        }

        // Check result code
        if (status == MED_STATUS_SUCCESS) {

            status = USBD_STATUS_SUCCESS;
        }
        else {

            TRACE_WARNING("LUN_Read: Cannot read media\n\r");
            status = USBD_STATUS_ABORTED;
        }
    }

    return status;
}
