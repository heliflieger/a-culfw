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

//-----------------------------------------------------------------------------
/// \unit
///
/// !Purpose
///
/// Implementation of DM9161 PHY driver
///
/// !Contents
///
/// Please refer to the list of functions in the #Overview# tab of this unit
/// for more detailed information.
//-----------------------------------------------------------------------------


// components/ethernet/dm9161/dm9161.h

#ifndef _DM9161_H
#define _DM9161_H

//-----------------------------------------------------------------------------
//         Headers
//-----------------------------------------------------------------------------
#include <pio/pio.h>

//-----------------------------------------------------------------------------
//         Definitions
//-----------------------------------------------------------------------------

/// The reset length setting for external reset configuration
#define DM9161_RESET_LENGTH         0xD

//-----------------------------------------------------------------------------
//         Types
//-----------------------------------------------------------------------------

/// The DM9161 instance
typedef struct _Dm9161 {

    /// The retry & timeout settings
    unsigned int retryMax;

    /// PHY address ( pre-defined by pins on reset )
    unsigned char phyAddress;

} Dm9161, *pDm9161;

//------------------------------------------------------------------------------
//         Exported functions
//------------------------------------------------------------------------------

extern void DM9161_SetupTimeout(Dm9161 *pDm, unsigned int toMax);

extern void DM9161_Init(Dm9161 *pDm, unsigned char phyAddress);

extern unsigned char DM9161_InitPhy(Dm9161 *pDm, 
                                    unsigned int mck,
                                    const Pin *pResetPins,
                                    unsigned int nbResetPins,
                                    const Pin *pEmacPins,
                                    unsigned int nbEmacPins);

extern unsigned char DM9161_AutoNegotiate(Dm9161 *pDm);

extern unsigned char DM9161_GetLinkSpeed(Dm9161 *pDm,
                                         unsigned char applySettings);

extern unsigned char DM9161_Send(Dm9161 *pDm,
                                 void *pBuffer,
                                 unsigned int size);

extern unsigned int DM9161_Poll(Dm9161 *pDm,
                                unsigned char *pBuffer,
                                unsigned int size);


#endif // #ifndef _DM9161_H

