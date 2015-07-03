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
/// !Purpose
///
/// The goal of FLASHD driver is to provide support for ATMEL Embedded Flash
/// devices. The FLASHD driver provides the unified interface of functions
/// and operations for all supported Embedded Flash Memory Devices. The driver
/// accesses the Flash memory by calling the low-level code which provided in 
/// EFC(EEFC) peripheral,o users do not have to concern themselves with the 
/// details of the special command sequences.  
///
/// !Usage
///
/// -# Initializes the %flash driver with FLASHD_Initialize().
/// -# Provide all neccessary operations such as %flash write, erase, lock, 
///    unlock, and set/clear GPNVM.
///    - They are able to check function arguments, address boundary etc.
///    - All %flash address argument is the unique address, as the result,
///      user no need to take care about the address is in which EFC, 
///      in which page and in which lock region for difference Flash device,
///      so that can help develop your uppon appcations with maximum efficiency.
/// -# Writes data in embedded %flash using FLASHD_Write().
/// -# Erase data in embedded %flash using FLASHD_Write().
/// -# Lock region in embedded %flash using FLASHD_Lock().
/// -# Unlock region in embedded %flash using FLASHD_Unlock().
/// -# Set GPNVM in embedded %flash using FLASHD_SetGPNVM().
/// -# Clear GPNVM in embedded %flash using FLASHD_ClearGPNVM().
///
/// !See also
///    - efc: EFC peripheral interface.
///    - eefc: EEFC peripheral interface.
//------------------------------------------------------------------------------
#ifndef FLASHD_H
#define FLASHD_H

//------------------------------------------------------------------------------
//         Functions
//------------------------------------------------------------------------------

extern void FLASHD_Initialize(unsigned int mck);

extern unsigned char FLASHD_Erase(void);

extern unsigned char FLASHD_Write(
    unsigned int address,
    const void *pBuffer,
    unsigned int size);

extern unsigned char FLASHD_Lock(
    unsigned int start,
    unsigned int end,
    unsigned int *pActualStart,
    unsigned int *pActualEnd);

extern unsigned char FLASHD_Unlock(
    unsigned int start,
    unsigned int end,
    unsigned int *pActualStart,
    unsigned int *pActualEnd);

extern unsigned char FLASHD_IsLocked(
    unsigned int start,
    unsigned int end);

extern unsigned char FLASHD_SetGPNVM(unsigned char gpnvm);

extern unsigned char FLASHD_ClearGPNVM(unsigned char gpnvm);

extern unsigned char FLASHD_IsGPNVMSet(unsigned char gpnvm);


#if defined(BOARD_FLASH_EFC)

extern unsigned char FLASHD_IsSecurityBitSet(void);

extern unsigned char FLASHD_SetSecurityBit(void);

#elif defined(BOARD_FLASH_EEFC)

#define FLASHD_IsSecurityBitSet() FLASHD_IsGPNVMSet(0)

#define FLASHD_SetSecurityBit()   FLASHD_SetGPNVM(0)


#endif //#if defined(BOARD_FLASH_EFC)


#endif //#ifndef FLASHD_H

