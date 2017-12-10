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

/*
    Title: USBGenericDescriptor implementation

    About: Purpose
        Implementation of the USBGenericDescriptor class.
*/

//------------------------------------------------------------------------------
//         Headers
//------------------------------------------------------------------------------

#include "USBGenericDescriptor.h"

//------------------------------------------------------------------------------
//         Exported functions
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Returns the length of a descriptor.
/// \param descriptor Pointer to a USBGenericDescriptor instance.
/// \return Length of descriptor in bytes.
//------------------------------------------------------------------------------
unsigned int USBGenericDescriptor_GetLength(
    const USBGenericDescriptor *descriptor)
{
    return descriptor->bLength;
}

//------------------------------------------------------------------------------
/// Returns the type of a descriptor.
/// \param descriptor Pointer to a USBGenericDescriptor instance.
/// \return Type of descriptor.
//------------------------------------------------------------------------------
unsigned char USBGenericDescriptor_GetType(
    const USBGenericDescriptor *descriptor)
{
    return descriptor->bDescriptorType;
}

//------------------------------------------------------------------------------
/// Returns a pointer to the descriptor right after the given one, when
/// parsing a Configuration descriptor.
/// \param descriptor - Pointer to a USBGenericDescriptor instance.
/// \return Pointer to the next descriptor.
//------------------------------------------------------------------------------
USBGenericDescriptor *USBGenericDescriptor_GetNextDescriptor(
    const USBGenericDescriptor *descriptor)
{
    return (USBGenericDescriptor *)
        (((char *) descriptor) + USBGenericDescriptor_GetLength(descriptor));
}
