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

#ifdef BOARD_FLASH_EFC

#include <efc/efc.h>
#include <utility/math.h>
#include <utility/assert.h>
#include <utility/trace.h>

#include <string.h>

//------------------------------------------------------------------------------
//         Local constants
//------------------------------------------------------------------------------

#if defined(AT91C_BASE_EFC) && !defined(AT91C_BASE_EFC0)
    #define AT91C_BASE_EFC0     AT91C_BASE_EFC
#endif

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
    AT91S_EFC *pStartEfc, *pEndEfc;
    unsigned short startPage, endPage;
    unsigned short numPagesInRegion;
    unsigned short actualStartPage, actualEndPage;

    // Convert start and end address in page numbers
    EFC_TranslateAddress(start, &pStartEfc, &startPage, 0);
    EFC_TranslateAddress(end, &pEndEfc, &endPage, 0);

    // Find out the first page of the first region to lock
    numPagesInRegion = AT91C_IFLASH_LOCK_REGION_SIZE / AT91C_IFLASH_PAGE_SIZE;
    actualStartPage = startPage - (startPage % numPagesInRegion);
    actualEndPage = endPage;
    if ((endPage % numPagesInRegion) != 0) {

        actualEndPage += numPagesInRegion - (endPage % numPagesInRegion);
    }

    // Store actual page numbers
    EFC_ComputeAddress(pStartEfc, actualStartPage, 0, pActualStart);
    EFC_ComputeAddress(pEndEfc, actualEndPage, 0, pActualEnd);
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
    EFC_SetMasterClock(mck);
    EFC_SetEraseBeforeProgramming(AT91C_BASE_EFC0, 1);
    EFC_DisableIt(AT91C_BASE_EFC0, AT91C_MC_FRDY | AT91C_MC_LOCKE | AT91C_MC_PROGE);
#ifdef AT91C_BASE_EFC1
    EFC_SetEraseBeforeProgramming(AT91C_BASE_EFC1, 1);
    EFC_DisableIt(AT91C_BASE_EFC1, AT91C_MC_FRDY | AT91C_MC_LOCKE | AT91C_MC_PROGE);
#endif
}

//------------------------------------------------------------------------------
/// Erases the entire flash.
/// Returns 0 if successful; otherwise returns an error code.
//------------------------------------------------------------------------------
unsigned char FLASHD_Erase(void)
{
    unsigned char error;

    error = EFC_PerformCommand(AT91C_BASE_EFC0, AT91C_MC_FCMD_ERASE_ALL, 0);
#ifdef AT91C_BASE_EFC1
    if (error) {

        return error;
    }
    error = EFC_PerformCommand(AT91C_BASE_EFC1, AT91C_MC_FCMD_ERASE_ALL, 0);
#endif

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
    AT91S_EFC *pEfc;
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
    EFC_TranslateAddress(address, &pEfc, &page, &offset);

    // Write all pages
    while (size > 0) {

        // Copy data in temporary buffer to avoid alignment problems
        writeSize = min(AT91C_IFLASH_PAGE_SIZE - offset, size);
        EFC_ComputeAddress(pEfc, page, 0, &pageAddress);
        padding = AT91C_IFLASH_PAGE_SIZE - offset - writeSize;

        // Pre-buffer data (mask with 0xFF)
        memcpy(pPageBuffer, (void *) pageAddress, offset);

        // Buffer data
        memcpy(pPageBuffer + offset, pBuffer, writeSize);

        // Post-buffer data
        memcpy(pPageBuffer + offset + writeSize, (void *) (pageAddress + offset + writeSize), padding);

        // Write page
        // Writing 8-bit and 16-bit data is not allowed 
        // and may lead to unpredictable data corruption
#ifdef EFC_EVEN_ODD_PROG
        // Write even words first with auto erase
        pAlignedDestination = (unsigned int*)pageAddress;
        pAlignedSource = (unsigned int*)pPageBuffer;
        sizeTmp = AT91C_IFLASH_PAGE_SIZE;
        while (sizeTmp >= 4) {

            *pAlignedDestination = *pAlignedSource;
            pAlignedDestination += 2;
            pAlignedSource += 2;
            sizeTmp -= 8;
        }
        // Send writing command
        error = EFC_PerformCommand(pEfc, AT91C_MC_FCMD_START_PROG, page);
        if (error) {

            return error;
        }

        // Then write odd words without auto erase
        EFC_SetEraseBeforeProgramming(AT91C_BASE_EFC0, 0);
#ifdef AT91C_BASE_EFC1
        EFC_SetEraseBeforeProgramming(AT91C_BASE_EFC1, 0);
#endif
        pAlignedDestination = (unsigned int*)pageAddress + 1;
        pAlignedSource = (unsigned int*)pPageBuffer + 1;
        sizeTmp = AT91C_IFLASH_PAGE_SIZE;
        while (sizeTmp >= 4) {

            *pAlignedDestination = *pAlignedSource;
            pAlignedDestination += 2;
            pAlignedSource += 2;
            sizeTmp -= 8;
        }

        // Send writing command
        error = EFC_PerformCommand(pEfc, AT91C_MC_FCMD_START_PROG, page);
        if (error) {

            return error;
        }

        EFC_SetEraseBeforeProgramming(AT91C_BASE_EFC0, 1);
#ifdef AT91C_BASE_EFC1
        EFC_SetEraseBeforeProgramming(AT91C_BASE_EFC1, 1);
#endif

#else 
        pAlignedDestination = (unsigned int*)pageAddress;
        pAlignedSource = (unsigned int*)pPageBuffer;        
        sizeTmp = AT91C_IFLASH_PAGE_SIZE;
        while (sizeTmp >= 4) {

            *pAlignedDestination++ = *pAlignedSource++;
            sizeTmp -= 4;
        }        
        
        // Send writing command
        error = EFC_PerformCommand(pEfc, AT91C_MC_FCMD_START_PROG, page);
        if (error) {

            return error;
        }
#endif
        // Progression
        address += AT91C_IFLASH_PAGE_SIZE;
        pBuffer = (void *) ((unsigned int) pBuffer + writeSize);
        size -= writeSize;
        page++;
        offset = 0;

#if defined(AT91C_BASE_EFC1)
        // Handle EFC crossover
        if ((pEfc == AT91C_BASE_EFC0) && (page >= (AT91C_IFLASH_NB_OF_PAGES / 2))) {

            pEfc = AT91C_BASE_EFC1;
            page = 0;
        }
#endif
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
    AT91S_EFC *pStartEfc, *pEndEfc, *pEfc;
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
    EFC_TranslateAddress(actualStart, &pStartEfc, &startPage, 0);
    EFC_TranslateAddress(actualEnd, &pEndEfc, &endPage, 0);

    // Lock all pages
    // If there is an EFC crossover, lock all pages from first EFC
#if defined(AT91C_BASE_EFC1)
    if (pStartEfc != pEndEfc) {

        while (startPage < (AT91C_IFLASH_NB_OF_PAGES / 2)) {

            error = EFC_PerformCommand(pStartEfc, AT91C_MC_FCMD_LOCK, startPage);
            if (error) {

                return error;
            }
            startPage += numPagesInRegion;
        }
        startPage = 0;
    }
#endif
    pEfc = pEndEfc;

    // Lock remaining pages
    while (startPage < endPage) {

        error = EFC_PerformCommand(pEfc, AT91C_MC_FCMD_LOCK, startPage);
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
    AT91S_EFC *pStartEfc, *pEndEfc, *pEfc;
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
    EFC_TranslateAddress(actualStart, &pStartEfc, &startPage, 0);
    EFC_TranslateAddress(actualEnd, &pEndEfc, &endPage, 0);

    // Unlock all pages
    // If there is an EFC crossover, unlock all pages from first EFC
#if defined(AT91C_BASE_EFC1)
    if (pStartEfc != pEndEfc) {

        while (startPage < (AT91C_IFLASH_NB_OF_PAGES / 2)) {

            error = EFC_PerformCommand(pStartEfc, AT91C_MC_FCMD_UNLOCK, startPage);
            if (error) {

                return error;
            }
            startPage += numPagesInRegion;
        }
        startPage = 0;
    }
#endif
    pEfc = pEndEfc;

    // Unlock remaining pages
    while (startPage < endPage) {

        error =  EFC_PerformCommand(pEfc, AT91C_MC_FCMD_UNLOCK, startPage);
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
    AT91S_EFC *pStartEfc, *pEndEfc, *pEfc;
    unsigned short startPage, endPage;
    unsigned char startRegion, endRegion;
    unsigned int numPagesInRegion;
    unsigned int numLockedRegions = 0;
    unsigned int status;
    
    // Get EFC & page values
    EFC_TranslateAddress(start, &pStartEfc, &startPage, 0);
    EFC_TranslateAddress(end, &pEndEfc, &endPage, 0);

    // Compute region indexes
    numPagesInRegion = AT91C_IFLASH_LOCK_REGION_SIZE / AT91C_IFLASH_PAGE_SIZE;
    startRegion = startPage / numPagesInRegion;
    endRegion = endPage / numPagesInRegion;
    if ((endPage % numPagesInRegion) != 0) {

        endRegion++;
    }

    // EFC cross-over, handle starting page -> end page of EFC0
#if defined(AT91C_BASE_EFC1)
    if (pStartEfc != pEndEfc) {

        status = EFC_GetStatus(pStartEfc);
        while (startRegion < 16) {

            if ((status & (1 << startRegion << 16)) != 0) {

                numLockedRegions++;
            }
            startRegion++;
        }
        startRegion = 0;
    }
#endif
    pEfc = pEndEfc;

    // Remaining regions / no EFC cross-over
    status = EFC_GetStatus(pEfc);
    while (startRegion < endRegion) {

        if ((status & (1 << startRegion << 16)) != 0) {

            numLockedRegions++;
        }
        startRegion++;
    }

    return numLockedRegions;
}

#if (EFC_NUM_GPNVMS > 0)
//------------------------------------------------------------------------------
/// Returns 1 if the given GPNVM bit is currently set; otherwise returns 0.
/// \param gpnvm  GPNVM bit index.
//------------------------------------------------------------------------------
unsigned char FLASHD_IsGPNVMSet(unsigned char gpnvm)
{
    AT91S_EFC *pEfc = AT91C_BASE_EFC0;
    unsigned int status;

    SANITY_CHECK(gpnvm < EFC_NUM_GPNVMS);

#ifdef AT91C_BASE_EFC1
    // GPNVM in EFC1
    if (gpnvm >= 8) {

        pEfc = AT91C_BASE_EFC1;
        gpnvm -= 8;
    }
#endif

    // Check if GPNVM is set
    status = EFC_GetStatus(pEfc);
    if ((status & (1 << gpnvm << 8)) != 0) {

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
    AT91S_EFC *pEfc = AT91C_BASE_EFC0;

    SANITY_CHECK(gpnvm < EFC_NUM_GPNVMS);

    if (!FLASHD_IsGPNVMSet(gpnvm)) {

#ifdef AT91C_BASE_EFC1
        // GPNVM in EFC1
        if (gpnvm >= 8) {

            pEfc = AT91C_BASE_EFC1;
            gpnvm -= 8;
        }
#endif

        return EFC_PerformCommand(pEfc, AT91C_MC_FCMD_SET_GP_NVM, gpnvm);
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
    AT91S_EFC *pEfc = AT91C_BASE_EFC0;

    SANITY_CHECK(gpnvm < EFC_NUM_GPNVMS);

    if (FLASHD_IsGPNVMSet(gpnvm)) {

#ifdef AT91C_BASE_EFC1
        // GPNVM in EFC1
        if (gpnvm >= 8) {

            pEfc = AT91C_BASE_EFC1;
            gpnvm -= 8;
        }
#endif

        return EFC_PerformCommand(pEfc, AT91C_MC_FCMD_CLR_GP_NVM, gpnvm);
    }
    else {

        return 0;
    }
}
#endif //#if (EFC_NUM_GPNVMS > 0)

#if !defined EFC_NO_SECURITY_BIT
//------------------------------------------------------------------------------
/// Returns 1 if the Security bit is currently set; otherwise returns 0.
//------------------------------------------------------------------------------
unsigned char FLASHD_IsSecurityBitSet(void)
{

    AT91S_EFC *pEfc = AT91C_BASE_EFC0;
    unsigned int status;

    status = EFC_GetStatus(pEfc);
    return ( ((status & AT91C_MC_SECURITY) != 0)?1:0 );
    
}

//------------------------------------------------------------------------------
/// Set Security Bit Command (SSB).
/// Returns 0 if successful; otherwise returns an error code.
//------------------------------------------------------------------------------
unsigned char FLASHD_SetSecurityBit(void)
{
    AT91S_EFC *pEfc = AT91C_BASE_EFC0;
 
    if( FLASHD_IsSecurityBitSet() == 0) {
        return EFC_PerformCommand(pEfc, AT91C_MC_FCMD_SET_SECURITY, 0);
    }
    else {
        return 0;
    }
}

#endif //#if (!defined EFC_NO_SECURITY_BIT)
#endif //#ifdef BOARD_FLASH_EFC

