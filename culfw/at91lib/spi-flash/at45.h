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
///
/// !!!Purpose
/// 
/// The Dataflash driver is based on top of the corresponding Spi driver.
/// A Dataflash structure instance has to be initialized using the DF_Init
/// function. Then basic dataflash operations can be launched using macros such
/// as DF_continuous_read. These macros invoke the DF_Command() function which
/// invokes the DPI low driver using the SPI_SendCommand() function.
/// Beware to compute the dataflash internal address, the dataflash sector
/// description must be known (DataflashDesc). Dataflash can be automatically
/// detected using the DF_Scan() function.
/// !!!Usage
/// 
/// -# Initializes an AT45 instance and configures SPI chip select pin
///    using AT45_Configure().
/// -# Detect DF and returns DF description corresponding to the device
///    connected using AT45_FindDevice().This function shall be called by 
///    the application before AT45_SendCommand.
/// -# Sends a command to the DF through the SPI using AT45_SendCommand().
///    The command is identified by its command code and the number of 
///    bytes to transfer.
///    -# Example code for sending command to write a page to DF.
/// \code
///    // Issue a page write through buffer 1 command
///	   error = AT45_SendCommand(pAt45, AT45_PAGE_WRITE_BUF1, 4, 
///	           pBuffer, size, address, 0, 0);
/// \endcode
///    -# Example code for sending command to read a page from DF.
///       If data needs to be received, then a data buffer must be 
///       provided.
/// \code
///    // Issue a continuous read array command
///    error = AT45_SendCommand(pAt45, AT45_CONTINUOUS_READ_LEG, 8,
///            pBuffer, size, address, 0, 0);
/// \endcode
///    -# This function does not block; its optional callback will 
///       be invoked when the transfer completes.
/// -# Check the AT45 driver is ready or not by polling AT45_IsBusy().
/// 
//------------------------------------------------------------------------------

#ifndef AT45_H
#define AT45_H

//------------------------------------------------------------------------------
//         Headers
//------------------------------------------------------------------------------

#include "spid.h"

//------------------------------------------------------------------------------
//         Definitions
//------------------------------------------------------------------------------

/// The dataflash driver is currently in use.
#define AT45_ERROR_LOCK         1
/// There was an error with the SPI driver.
#define AT45_ERROR_SPI          2

/// AT45 dataflash SPI CSR settings given MCK and SPCK.
#define AT45_CSR(mck, spck) \
    (AT91C_SPI_NCPHA | SPID_CSR_DLYBCT(mck, 250) \
     | SPID_CSR_DLYBS(mck, 250) | SPID_CSR_SCBR(mck, spck))

//------------------------------------------------------------------------------
//         Macros
//------------------------------------------------------------------------------

#define AT45_PageOffset(pAt45) ((pAt45)->pDesc->pageOffset)
#define AT45_PageNumber(pAt45) ((pAt45)->pDesc->pageNumber)

/// Returns 1 if the device is ready; otherwise 0.
#define AT45_STATUS_READY(status)       (status & 0x80)
/// Returns the device ID code.
#define AT45_STATUS_ID(status)          (status & 0x3c)
/// Returns 1 if the device is configured in binary page mode; otherwise 0.
#define AT45_STATUS_BINARY(status)      (status & 0x01)

//------------------------------------------------------------------------------
//         Definitions
//------------------------------------------------------------------------------

/// Main memory page read command code.
#define AT45_PAGE_READ              0xD2
/// Continous array read (legacy) command code.
#define AT45_CONTINUOUS_READ_LEG    0xE8
/// Continous array read (low frequency) command code.
#define AT45_CONTINUOUS_READ_LF     0x03
/// Continous array read command code.
#define AT45_CONTINUOUS_READ        0x0B
/// Buffer 1 read (low frequency) command code.
#define AT45_BUF1_READ_LF           0xD1
/// Buffer 2 read (low frequency) command code.
#define AT45_BUF2_READ_LF           0xD3
/// Buffer 1 read (serial) command code.
#define AT45_BUF1_READ_SER          0xD4
/// Buffer 2 read (serial) command code.
#define AT45_BUF2_READ_SER          0xD6
/// Buffer 1 read (8-bit) command code.
#define AT45_BUF1_READ_8B           0x54
/// Buffer 2 read (8-bit) command code.
#define AT45_BUF2_READ_8B           0x56

/// Buffer 1 write command code.
#define AT45_BUF1_WRITE             0x84
/// Buffer 2 write command code.
#define AT45_BUF2_WRITE             0x87
/// Buffer 1 to main memory page program with erase command code.
#define AT45_BUF1_MEM_ERASE         0x83
/// Buffer 2 to main memory page program with erase command code.
#define AT45_BUF2_MEM_ERASE         0x86
/// Buffer 1 to main memory page program without erase command code.
#define AT45_BUF1_MEM_NOERASE       0x88
/// Buffer 2 to main memory page program without erase command code.
#define AT45_BUF2_MEM_NOERASE       0x89
/// Page erase command code.
#define AT45_PAGE_ERASE             0x81
/// Block erase command code.
#define AT45_BLOCK_ERASE            0x50
/// Sector erase command code.
#define AT45_SECTOR_ERASE           0x7C
/// Chip erase command code.
#define AT45_CHIP_ERASE             0xC7, 0x94, 0x80, 0x9A
/// Main memory page program through buffer 1 command code.
#define AT45_PAGE_WRITE_BUF1        0x82
/// Main memory page program through buffer 2 command code.
#define AT45_PAGE_WRITE_BUF2        0x85

/// Main memory page to buffer 1 transfer command code.
#define AT45_PAGE_BUF1_TX           0x53
/// Main memory page to buffer 2 transfer command code.
#define AT45_PAGE_BUF2_TX           0x55
/// Main memory page to buffer 1 compare command code.
#define AT45_PAGE_BUF1_CMP          0x60
/// Main memory page to buffer 2 compare command code.
#define AT45_PAGE_BUF2_CMP          0x61
/// Auto page rewrite through buffer 1 command code.
#define AT45_AUTO_REWRITE_BUF1      0x58
/// Auto page rewrite through buffer 2 command code.
#define AT45_AUTO_REWRITE_BUF2      0x59
/// Deep power-down command code.
#define AT45_DEEP_PDOWN             0xB9
/// Resume from deep power-down command code.
#define AT45_RES_DEEP_PDOWN         0xAB
/// Status register read command code.
#define AT45_STATUS_READ            0xD7
/// Manufacturer and device ID read command code.
#define AT45_ID_READ                0x9F

/// Power-of-2 binary page size configuration command code.
#define AT45_BINARY_PAGE_FIRST_OPCODE   0x3D
#define AT45_BINARY_PAGE                0x2A, 0x80, 0xA6

//------------------------------------------------------------------------------
//         Types
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Dataflash description. A constant array of DataflashDesc instance is defined
/// in at45.c. The DF_Scan() function returns the corresponding descriptor
/// according to the dataflash ID detected.
/// This description (page_size, page_offset) is used to compute the internal
/// dataflash address by the DF_Command() function.
//------------------------------------------------------------------------------
typedef struct {

    /// dataflash page number.
	unsigned int pageNumber;
    // indicate if power-of-2 binary page supported.
	unsigned int hasBinaryPage;
    /// dataflash page size.
	unsigned int pageSize;
    /// page offset in command.
	unsigned int pageOffset;
    /// Dataflash ID.
	unsigned char id;
    /// Identifier.
	const char *name;

} At45Desc;

//------------------------------------------------------------------------------
/// Dataflash driver structure. It holds the current command being processed.
/// This structure is initialized by the DF_Init() command.
/// pDfDesc field can be initialized by the DF_Scan() function.
/// cmdBuffer is a private driver area used to compute the dataflash address to
/// be sent to the dataflash. 
/// Beware the PDC master must have access to this area.
//------------------------------------------------------------------------------
typedef struct _Dataflash {

    /// Pointer to Spi Structure (SPI low level driver).
	Spid *pSpid;
    /// Current SPI command sent to the SPI low level driver.
	SpidCmd command;
    /// Pointer to the dataflash description.
	const At45Desc *pDesc;
    /// Buffer to store the current command (opcode + dataflash address.
	unsigned char pCmdBuffer[8];

} At45;

//------------------------------------------------------------------------------
//         Exported functions
//------------------------------------------------------------------------------

extern unsigned char AT45_Configure(At45 *pAt45, Spid *pSpid, unsigned char spiCs);

extern unsigned char AT45_IsBusy(At45 *pAt45);

extern unsigned char AT45_SendCommand(
	At45 *pAt45,
	unsigned char cmd,
	unsigned char cmdSize,
	unsigned char *pData,
	unsigned int dataSize,
	unsigned int address,
	SpidCallback callback,
	void *pArgument);

extern const At45Desc * AT45_FindDevice(At45 *pAt45, unsigned char status);

extern unsigned int  AT45_PageSize(At45 *pAt45);
#endif // #ifndef AT45_H

