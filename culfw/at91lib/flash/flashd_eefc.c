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

#include "flashd.h"
#include <board.h>

#ifdef BOARD_FLASH_EEFC

#include <eefc/eefc.h>
#include <utility/math.h>
#include <utility/assert.h>
#include <utility/trace.h>

#include <string.h>

//------------------------------------------------------------------------------
//         Local functions
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Computes the lock range associated with the given address range.
/// \param start  Start address of lock range.
/// \param end  End address of lock range.
/// \param pActualStart  Actual start address of lock range.
/// \param pActualEnd  Actual end address of lock range.
//------------------------------------------------------------------------------
static void ComputeLockRange(
    unsigned int start,
    unsigned int end,
    unsigned int *pActualStart,
    unsigned int *pActualEnd)
{
    unsigned short startPage, endPage;
    unsigned short numPagesInRegion;
    unsigned short actualStartPage, actualEndPage;

    // Convert start and end address in page numbers
    EFC_TranslateAddress(start, &startPage, 0);
    EFC_TranslateAddress(end, &endPage, 0);

    // Find out the first page of the first region to lock
    numPagesInRegion = AT91C_IFLASH_LOCK_REGION_SIZE / AT91C_IFLASH_PAGE_SIZE;
    actualStartPage = startPage - (startPage % numPagesInRegion);
    actualEndPage = endPage;
    if ((endPage % numPagesInRegion) != 0) {

        actualEndPage += numPagesInRegion - (endPage % numPagesInRegion);
    }

    // Store actual page numbers
    EFC_ComputeAddress(actualStartPage, 0, pActualStart);
    EFC_ComputeAddress(actualEndPage, 0, pActualEnd);
    TRACE_DEBUG("Actual lock range is 0x%06X - 0x%06X\n\r", *pActualStart, *pActualEnd);
}

//------------------------------------------------------------------------------
//         Global functions
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Initializes the flash driver.
/// \param mck  Master clock frequency in Hz.
//------------------------------------------------------------------------------
void FLASHD_Initialize(unsigned int mck)
{
    EFC_DisableFrdyIt();
}

//------------------------------------------------------------------------------
/// Erases the entire flash.
/// Returns 0 if successful; otherwise returns an error code.
//------------------------------------------------------------------------------
unsigned char FLASHD_Erase(void)
{
    unsigned char error;

    error = EFC_PerformCommand(AT91C_EFC_FCMD_EA, 0);

    return error;
}

static unsigned char pPageBuffer[AT91C_IFLASH_PAGE_SIZE];

//------------------------------------------------------------------------------
/// Writes a data buffer in the internal flash. This function works in polling
/// mode, and thus only returns when the data has been effectively written.
/// Returns 0 if successful; otherwise returns an error code.
/// \param address  Write address.
/// \param pBuffer  Data buffer.
/// \param size  Size of data buffer in bytes.
//------------------------------------------------------------------------------
unsigned char FLASHD_Write(
    unsigned int address,
    const void *pBuffer,
    unsigned int size)
{
    unsigned short page;
    unsigned short offset;
    unsigned int writeSize;
    unsigned int pageAddress;
    unsigned short padding;
    unsigned char error;

    unsigned int sizeTmp;
    unsigned int *pAlignedDestination; 
    unsigned int *pAlignedSource;
    
    SANITY_CHECK(address >= AT91C_IFLASH);
    SANITY_CHECK(pBuffer);
    SANITY_CHECK((address + size) <= (AT91C_IFLASH + AT91C_IFLASH_SIZE));

    // Translate write address
    EFC_TranslateAddress(address, &page, &offset);

    // Write all pages
    while (size > 0) {

        // Copy data in temporary buffer to avoid alignment problems
        writeSize = min(AT91C_IFLASH_PAGE_SIZE - offset, size);
        EFC_ComputeAddress(page, 0, &pageAddress);
        padding = AT91C_IFLASH_PAGE_SIZE - offset - writeSize;

        // Pre-buffer data
        memcpy(pPageBuffer, (void *) pageAddress, offset);

        // Buffer data
        memcpy(pPageBuffer + offset, pBuffer, writeSize);

        // Post-buffer data
        memcpy(pPageBuffer + offset + writeSize, (void *) (pageAddress + offset + writeSize), padding);

        // Write page
        // Writing 8-bit and 16-bit data is not allowed 
        // and may lead to unpredictable data corruption
        pAlignedDestination = (unsigned int*)pageAddress;
        pAlignedSource = (unsigned int*)pPageBuffer;        
        sizeTmp = AT91C_IFLASH_PAGE_SIZE;
        while (sizeTmp >= 4) {

            *pAlignedDestination++ = *pAlignedSource++;
            sizeTmp -= 4;
        }        
               
        // Send writing command
        error = EFC_PerformCommand(AT91C_EFC_FCMD_EWP, page);
        if (error) {

            return error;
        }

        // Progression
        address += AT91C_IFLASH_PAGE_SIZE;
        pBuffer = (void *) ((unsigned int) pBuffer + writeSize);
        size -= writeSize;
        page++;
        offset = 0;
    }

    return 0;
}

//------------------------------------------------------------------------------
/// Locks all the regions in the given address range. The actual lock range is
/// reported through two output parameters.
/// Returns 0 if successful; otherwise returns an error code.
/// \param start  Start address of lock range.
/// \param end  End address of lock range.
/// \param pActualStart  Start address of the actual lock range (optional).
/// \param pActualEnd  End address of the actual lock range (optional).
//------------------------------------------------------------------------------
unsigned char FLASHD_Lock(
    unsigned int start,
    unsigned int end,
    unsigned int *pActualStart,
    unsigned int *pActualEnd)
{
    unsigned int actualStart, actualEnd;
    unsigned short startPage, endPage;
    unsigned char error;
    unsigned short numPagesInRegion = AT91C_IFLASH_LOCK_REGION_SIZE / AT91C_IFLASH_PAGE_SIZE;

    // Compute actual lock range and store it
    ComputeLockRange(start, end, &actualStart, &actualEnd);
    if (pActualStart) {

        *pActualStart = actualStart;
    }
    if (pActualEnd) {

        *pActualEnd = actualEnd;
    }

    // Compute page numbers
    EFC_TranslateAddress(actualStart, &startPage, 0);
    EFC_TranslateAddress(actualEnd, &endPage, 0);

    // Lock all pages
    while (startPage < endPage) {

        error = EFC_PerformCommand(AT91C_EFC_FCMD_SLB, startPage);
        if (error) {

            return error;
        }
        startPage += numPagesInRegion;
    }

    return 0;
}

//------------------------------------------------------------------------------
/// Unlocks all the regions in the given address range. The actual unlock range is
/// reported through two output parameters.
/// Returns 0 if successful; otherwise returns an error code.
/// \param start  Start address of unlock range.
/// \param end  End address of unlock range.
/// \param pActualStart  Start address of the actual unlock range (optional).
/// \param pActualEnd  End address of the actual unlock range (optional).
//------------------------------------------------------------------------------
unsigned char FLASHD_Unlock(
    unsigned int start,
    unsigned int end,
    unsigned int *pActualStart,
    unsigned int *pActualEnd)
{
    unsigned int actualStart, actualEnd;
    unsigned short startPage, endPage;
    unsigned char error;
    unsigned short numPagesInRegion = AT91C_IFLASH_LOCK_REGION_SIZE / AT91C_IFLASH_PAGE_SIZE;

    // Compute actual unlock range and store it
    ComputeLockRange(start, end, &actualStart, &actualEnd);
    if (pActualStart) {

        *pActualStart = actualStart;
    }
    if (pActualEnd) {

        *pActualEnd = actualEnd;
    }

    // Compute page numbers
    EFC_TranslateAddress(actualStart, &startPage, 0);
    EFC_TranslateAddress(actualEnd, &endPage, 0);

    // Unlock all pages
    while (startPage < endPage) {

        error = EFC_PerformCommand(AT91C_EFC_FCMD_CLB, startPage);
        if (error) {

            return error;
        }
        startPage += numPagesInRegion;
    }

    return 0;
}

//------------------------------------------------------------------------------
/// Returns the number of locked regions inside the given address range.
/// \param start  Start address of range.
/// \param end  End address of range.
//------------------------------------------------------------------------------
unsigned char FLASHD_IsLocked(unsigned int start, unsigned int end)
{
    unsigned short startPage, endPage;
    unsigned char startRegion, endRegion;
    unsigned int numPagesInRegion;
    unsigned int status;
    unsigned char error;
    unsigned int numLockedRegions = 0;

    // Compute page numbers
    EFC_TranslateAddress(start, &startPage, 0);
    EFC_TranslateAddress(end, &endPage, 0);

    // Compute region numbers
    numPagesInRegion = AT91C_IFLASH_LOCK_REGION_SIZE / AT91C_IFLASH_PAGE_SIZE;
    startRegion = startPage / numPagesInRegion;
    endRegion = endPage / numPagesInRegion;
    if ((endPage % numPagesInRegion) != 0) {

        endRegion++;
    }

    // Retrieve lock status
    error = EFC_PerformCommand(AT91C_EFC_FCMD_GLB, 0);
    ASSERT(!error, "-F- Error while trying to fetch lock bits status (0x%02X)\n\r", error);
    status = EFC_GetResult();

    // Check status of each involved region
    while (startRegion < endRegion) {

        if ((status & (1 << startRegion)) != 0) {

            numLockedRegions++;
        }
        startRegion++;
    }

    return numLockedRegions;
}

//------------------------------------------------------------------------------
/// Returns 1 if the given GPNVM bit is currently set; otherwise returns 0.
/// \param gpnvm  GPNVM bit index.
//------------------------------------------------------------------------------
unsigned char FLASHD_IsGPNVMSet(unsigned char gpnvm)
{
    unsigned char error;
    unsigned int status;

    SANITY_CHECK(gpnvm < EFC_NUM_GPNVMS);

    // Get GPNVMs status
    error = EFC_PerformCommand(AT91C_EFC_FCMD_GFB, 0);
    ASSERT(!error, "-F- Error while trying to fetch GPNVMs status (0x%02X)\n\r", error);
    status = EFC_GetResult();

    // Check if GPNVM is set
    if ((status & (1 << gpnvm)) != 0) {

        return 1;
    }
    else {

        return 0;
    }
}

//------------------------------------------------------------------------------
/// Sets the selected GPNVM bit.
/// Returns 0 if successful; otherwise returns an error code.
/// \param gpnvm  GPNVM index.
//------------------------------------------------------------------------------
unsigned char FLASHD_SetGPNVM(unsigned char gpnvm)
{
    SANITY_CHECK(gpnvm < EFC_NUM_GPNVMS);

    if (!FLASHD_IsGPNVMSet(gpnvm)) {

        return EFC_PerformCommand(AT91C_EFC_FCMD_SFB, gpnvm);
    }
    else {

        return 0;
    }
}

//------------------------------------------------------------------------------
/// Clears the selected GPNVM bit.
/// Returns 0 if successful; otherwise returns an error code.
/// \param gpnvm  GPNVM index.
//------------------------------------------------------------------------------
unsigned char FLASHD_ClearGPNVM(unsigned char gpnvm)
{
    SANITY_CHECK(gpnvm < EFC_NUM_GPNVMS);

    if (FLASHD_IsGPNVMSet(gpnvm)) {

        return EFC_PerformCommand(AT91C_EFC_FCMD_CFB, gpnvm);
    }
    else {

        return 0;
    }
}

#endif //#ifdef BOARD_FLASH_EEFC
