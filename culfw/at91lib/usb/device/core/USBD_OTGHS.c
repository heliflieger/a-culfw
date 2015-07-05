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

/*!
    Functions for OTGHS peripheral usage.
*/
 
//------------------------------------------------------------------------------
//      Headers
//------------------------------------------------------------------------------

#include <board.h>

#ifdef CHIP_OTGHS

#include "common.h"
#include "trace.h"
#include "usb.h"

//------------------------------------------------------------------------------
//         Definitions
//------------------------------------------------------------------------------

#define NUM_IT_MAX       (AT91C_BASE_OTGHS->OTGHS_IPFEATURES & AT91C_OTGHS_EPT_NBR_MAX)
#define NUM_IT_MAX_DMA   ((AT91C_BASE_OTGHS->OTGHS_IPFEATURES & AT91C_OTGHS_DMA_CHANNEL_NBR)>>4)

#define SHIFT_DMA         24
#define SHIFT_INTERUPT    12

#define DMA

//------------------------------------------------------------------------------
//      Structures
//------------------------------------------------------------------------------

// \brief  Endpoint states
typedef enum {

    endpointStateDisabled,
    endpointStateIdle,
    endpointStateWrite,
    endpointStateRead,
    endpointStateHalted

} EndpointState_t;

//------------------------------------------------------------------------------
//      Macros
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//      Internal Functions
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// \brief  Returns a pointer to the OTGHS controller interface used by an USB
//         driver
//
//         The pointer is cast to the correct type (AT91PS_OTGHS).
// \param  pUsb Pointer to a S_usb instance
// \return Pointer to the USB controller interface
// \see    S_usb
//------------------------------------------------------------------------------
static AT91PS_OTGHS OTGHS_GetDriverInterface(const S_usb *pUsb)
{
    return (AT91PS_OTGHS) pUsb->pDriver->pInterface;
}

//------------------------------------------------------------------------------
// \fn      OTGHS_GetInterfaceEPT
// \brief   Returns OTGHS endpoint FIFO interface from S_usb structure
//------------------------------------------------------------------------------
static AT91PS_OTGHS_EPTFIFO OTGHS_GetInterfaceEPT(const S_usb *pUsb) 
{
    return (AT91PS_OTGHS_EPTFIFO) pUsb->pDriver->pEndpointFIFO;
}


//------------------------------------------------------------------------------
// \brief  Enables the peripheral clock of the USB controller associated with
//         the specified USB driver
// \param  pUsb Pointer to a S_usb instance
// \see    S_usb
//------------------------------------------------------------------------------
static void OTGHS_EnableMCK(const S_usb *pUsb)
{

}

//------------------------------------------------------------------------------
// \brief  Disables the peripheral clock of the USB controller associated with
//         the specified USB driver
// \param  pUsb Pointer to a S_usb instance
// \see    S_usb
//------------------------------------------------------------------------------
static void OTGHS_DisableMCK(const S_usb *pUsb)
{

}

//------------------------------------------------------------------------------
// \brief  Enables the 48MHz clock of the USB controller associated with
//         the specified USB driver
// \param  pUsb Pointer to a S_usb instance
// \see    S_usb
//------------------------------------------------------------------------------
static void OTGHS_EnableOTGHSCK(const S_usb *pUsb)
{

}

//------------------------------------------------------------------------------
// \brief  Disables the 48MHz clock of the USB controller associated with
//         the specified USB driver
// \param  pUsb Pointer to a S_usb instance
// \see    S_usb
//------------------------------------------------------------------------------
static void OTGHS_DisableOTGHSCK(const S_usb *pUsb)
{

}

//------------------------------------------------------------------------------
// \brief  Enables the transceiver of the USB controller associated with
//         the specified USB driver
// \param  pUsb Pointer to a S_usb instance
// \see    S_usb
//------------------------------------------------------------------------------
static void OTGHS_EnableTransceiver(const S_usb *pUsb)
{
    SET(OTGHS_GetDriverInterface(pUsb)->OTGHS_CTRL, AT91C_OTGHS_OTGPADE);
}

//------------------------------------------------------------------------------
// \brief  Disables the transceiver of the USB controller associated with
//         the specified USB driver
// \param  pUsb Pointer to a S_usb instance
// \see    S_usb
//------------------------------------------------------------------------------
static void OTGHS_DisableTransceiver(const S_usb *pUsb)
{
    CLEAR(OTGHS_GetDriverInterface(pUsb)->OTGHS_CTRL, AT91C_OTGHS_OTGPADE);
}

//------------------------------------------------------------------------------
// \brief  Invokes the callback associated with a finished transfer on an
//         endpoint
// \param  pEndpoint Pointer to a S_usb_endpoint instance
// \param  bStatus   Status code returned by the transfer operation
// \see    Status codes
// \see    S_usb_endpoint
//------------------------------------------------------------------------------
static void OTGHS_EndOfTransfer(S_usb_endpoint *pEndpoint,
                                         char bStatus)
{
    if ((pEndpoint->dState == endpointStateWrite)
        || (pEndpoint->dState == endpointStateRead)) {

        TRACE_DEBUG_WP("E");

        // Endpoint returns in Idle state
        pEndpoint->dState = endpointStateIdle;

        // Invoke callback is present
        if (pEndpoint->fCallback != 0) {

            pEndpoint->fCallback((unsigned int) pEndpoint->pArgument,
                                 (unsigned int) bStatus,
                                 pEndpoint->dBytesTransferred,
                                 pEndpoint->dBytesRemaining
                                 + pEndpoint->dBytesBuffered);
        }
    }
}

//------------------------------------------------------------------------------
// \brief  Transfers a data payload from the current tranfer buffer to the
//         endpoint FIFO.
// \param  pUsb      Pointer to a S_usb instance
// \param  bEndpoint Index of endpoint
// \return Number of bytes transferred
// \see    S_usb
//------------------------------------------------------------------------------
static unsigned int OTGHS_WritePayload(const S_usb *pUsb,
                                       unsigned char bEndpoint)
{
    AT91PS_OTGHS_EPTFIFO pInterfaceEPT = OTGHS_GetInterfaceEPT(pUsb);
    S_usb_endpoint *pEndpoint = USB_GetEndpoint(pUsb, bEndpoint);
    char           *pfifo;
    unsigned int   dBytes;
    unsigned int   dCtr;

    pfifo = (char*)&(pInterfaceEPT->OTGHS_READEPT0[bEndpoint*16384]);

    // Get the number of bytes to send
    dBytes = min(pEndpoint->wMaxPacketSize, pEndpoint->dBytesRemaining);

    // Transfer one packet in the FIFO buffer
    for (dCtr = 0; dCtr < dBytes; dCtr++) {

        pfifo[dCtr] = *(pEndpoint->pData);
        pEndpoint->pData++;
    }

    pEndpoint->dBytesBuffered += dBytes;
    pEndpoint->dBytesRemaining -= dBytes;

    return dBytes;
}

//----------------------------------------------------------------------------
// \brief  Transfers a data payload from an endpoint FIFO to the current
//         transfer buffer.
// \param  pUsb        Pointer to a S_usb instance
// \param  bEndpoint   Index of endpoint
// \param  wPacketSize Size of received data packet
// \return Number of bytes transferred
// \see    S_usb
//------------------------------------------------------------------------------
static unsigned int OTGHS_GetPayload(const S_usb    *pUsb,
                                     unsigned char  bEndpoint,
                                     unsigned short wPacketSize)
{
    AT91PS_OTGHS_EPTFIFO pInterfaceEPT = OTGHS_GetInterfaceEPT(pUsb);
    S_usb_endpoint *pEndpoint = USB_GetEndpoint(pUsb, bEndpoint);
    char           *pfifo;
    unsigned int   dBytes;
    unsigned int   dCtr;

    pfifo = (char*)&(pInterfaceEPT->OTGHS_READEPT0[bEndpoint*16384]);

    // Get number of bytes to retrieve
    dBytes = min(pEndpoint->dBytesRemaining, wPacketSize);

    // Retrieve packet
    for (dCtr = 0; dCtr < dBytes; dCtr++) {

        *(pEndpoint->pData) = pfifo[dCtr];
        pEndpoint->pData++;
    }

    pEndpoint->dBytesRemaining -= dBytes;
    pEndpoint->dBytesTransferred += dBytes;
    pEndpoint->dBytesBuffered += wPacketSize - dBytes;

    return dBytes;
}

//------------------------------------------------------------------------------
// \brief  Transfers a received SETUP packet from endpoint 0 FIFO to the
//         S_usb_request structure of an USB driver
// \param  pUsb Pointer to a S_usb instance
// \see    S_usb
//------------------------------------------------------------------------------
static void OTGHS_GetSetup(const S_usb *pUsb)
{
    unsigned int *pData = (unsigned int *) USB_GetSetup(pUsb);
    AT91PS_OTGHS_EPTFIFO pInterfaceEPT = OTGHS_GetInterfaceEPT(pUsb);

    pData[0] = pInterfaceEPT->OTGHS_READEPT0[0];
    pData[1] = pInterfaceEPT->OTGHS_READEPT0[0];
}

//------------------------------------------------------------------------------
// \brief  This function reset all endpoint transfer descriptors
// \param  pUsb Pointer to a S_usb instance
// \see    S_usb
//------------------------------------------------------------------------------
static void OTGHS_ResetEndpoints(const S_usb *pUsb)
{
    S_usb_endpoint *pEndpoint;
    unsigned char  bEndpoint;

    // Reset the transfer descriptor of every endpoint
    for (bEndpoint = 0; bEndpoint < pUsb->dNumEndpoints; bEndpoint++) {

        pEndpoint = USB_GetEndpoint(pUsb, bEndpoint);

        // Reset endpoint transfer descriptor
        pEndpoint->pData = 0;
        pEndpoint->dBytesRemaining = 0;
        pEndpoint->dBytesTransferred = 0;
        pEndpoint->dBytesBuffered = 0;
        pEndpoint->fCallback = 0;
        pEndpoint->pArgument = 0;

        // Configure endpoint characteristics
        pEndpoint->dState = endpointStateDisabled;
    }
}

//------------------------------------------------------------------------------
// \brief  Disable all endpoints (except control endpoint 0), aborting current
//         transfers if necessary.
// \param  pUsb Pointer to a S_usb instance
//------------------------------------------------------------------------------
static void OTGHS_DisableEndpoints(const S_usb *pUsb)
{
    S_usb_endpoint *pEndpoint;
    unsigned char  bEndpoint;

    // Foreach endpoint, if it is enabled, disable it and invoke the callback
    // Control endpoint 0 is not disabled
    for (bEndpoint = 1; bEndpoint < pUsb->dNumEndpoints; bEndpoint++) {

        pEndpoint = USB_GetEndpoint(pUsb, bEndpoint);
        OTGHS_EndOfTransfer(pEndpoint, USB_STATUS_RESET);

        pEndpoint->dState = endpointStateDisabled;
    }
}

//------------------------------------------------------------------------------
// \brief  Endpoint interrupt handler.
//
//         Handle IN/OUT transfers, received SETUP packets and STALLing
// \param  pUsb      Pointer to a S_usb instance
// \param  bEndpoint Index of endpoint
// \see    S_usb
//------------------------------------------------------------------------------
static void OTGHS_EndpointHandler(const S_usb *pUsb, unsigned char bEndpoint)
{
    S_usb_endpoint *pEndpoint = USB_GetEndpoint(pUsb, bEndpoint);
    AT91PS_OTGHS pInterface = OTGHS_GetDriverInterface(pUsb);
    unsigned int dStatus = pInterface->OTGHS_DEVEPTCSR[bEndpoint];
    unsigned short wPacketSize;

    TRACE_DEBUG_WP("Ept%d, 0x%X ", bEndpoint, dStatus);

    // Handle interrupts
    // IN packet sent
    if((ISSET(pInterface->OTGHS_DEVEPTCMR[bEndpoint], AT91C_OTGHS_TXINI))
    && (ISSET(dStatus, AT91C_OTGHS_TXINI ))) {

        TRACE_DEBUG_WP("Wr ");

        if (pEndpoint->dBytesBuffered > 0) {

            TRACE_DEBUG_WP("%d ", pEndpoint->dBytesBuffered);

            pEndpoint->dBytesTransferred += pEndpoint->dBytesBuffered;
            pEndpoint->dBytesBuffered = 0;
        }

        if ((!pEndpoint->isDataSent) || (pEndpoint->dBytesRemaining > 0)) {
            
            OTGHS_WritePayload(pUsb, bEndpoint);
            pEndpoint->isDataSent = true;

            pInterface->OTGHS_DEVEPTCCR[bEndpoint] = AT91C_OTGHS_TXINI;
            // For a non-control endpoint, the FIFOCON bit must be cleared
            // to start the transfer
            if ((AT91C_OTGHS_EPT_TYPE & pInterface->OTGHS_DEVEPTCFG[bEndpoint])
                != AT91C_OTGHS_EPT_TYPE_CTL_EPT) {

                pInterface->OTGHS_DEVEPTCDR[bEndpoint] = AT91C_OTGHS_FIFOCON;
            }
        }
        else {
            
            pInterface->OTGHS_DEVEPTCDR[bEndpoint] = AT91C_OTGHS_TXINI;

            // Disable interrupt if this is not a control endpoint
            if ((AT91C_OTGHS_EPT_TYPE & pInterface->OTGHS_DEVEPTCFG[bEndpoint])
                != AT91C_OTGHS_EPT_TYPE_CTL_EPT) {

                pInterface->OTGHS_DEVIDR = 1<<SHIFT_INTERUPT<<bEndpoint;

            }
            OTGHS_EndOfTransfer(pEndpoint, USB_STATUS_SUCCESS);
        }
    }

    // OUT packet received
    if(ISSET(dStatus, AT91C_OTGHS_RXOUT)) {

        TRACE_DEBUG_WP("Rd ");

        // Check that the endpoint is in Read state
        if (pEndpoint->dState != endpointStateRead) {

            // Endpoint is NOT in Read state
            if (ISCLEARED(pInterface->OTGHS_DEVEPTCFG[bEndpoint], AT91C_OTGHS_EPT_TYPE)
             && ISCLEARED(dStatus, (0x7FF<<20))) {  // byte count

                // Control endpoint, 0 bytes received
                // Acknowledge the data and finish the current transfer
                TRACE_DEBUG_WP("Ack ");
                pInterface->OTGHS_DEVEPTCCR[bEndpoint] = AT91C_OTGHS_RXOUT;

                OTGHS_EndOfTransfer(pEndpoint, USB_STATUS_SUCCESS);
            }
            else if (ISSET(dStatus, AT91C_OTGHS_STALL)) {

                // Non-control endpoint
                // Discard stalled data
                TRACE_DEBUG_WP("Disc ");
                pInterface->OTGHS_DEVEPTCCR[bEndpoint] = AT91C_OTGHS_RXOUT;
            }
            else {

                // Non-control endpoint
                // Nak data
                TRACE_DEBUG_WP("Nak ");
                pInterface->OTGHS_DEVIDR = 1<<SHIFT_INTERUPT<<bEndpoint;
            }
        }
        else {

            // Endpoint is in Read state
            // Retrieve data and store it into the current transfer buffer
            wPacketSize = (unsigned short) ((dStatus >> 20) & 0x7FF);

            TRACE_DEBUG_WP("%d ", wPacketSize);

            OTGHS_GetPayload(pUsb, bEndpoint, wPacketSize);

            pInterface->OTGHS_DEVEPTCCR[bEndpoint] = AT91C_OTGHS_RXOUT;
            pInterface->OTGHS_DEVEPTCDR[bEndpoint] = AT91C_OTGHS_FIFOCON;

            if ((pEndpoint->dBytesRemaining == 0)
                || (wPacketSize < pEndpoint->wMaxPacketSize)) {

                pInterface->OTGHS_DEVEPTCDR[bEndpoint] = AT91C_OTGHS_RXOUT;

                // Disable interrupt if this is not a control endpoint
                if ((AT91C_OTGHS_EPT_TYPE & pInterface->OTGHS_DEVEPTCFG[bEndpoint])
                    != AT91C_OTGHS_EPT_TYPE_CTL_EPT) {

                    pInterface->OTGHS_DEVIDR = 1<<SHIFT_INTERUPT<<bEndpoint;
                }

                OTGHS_EndOfTransfer(pEndpoint, USB_STATUS_SUCCESS);
            }
        }
    }

    // SETUP packet received
    if(ISSET(dStatus, AT91C_OTGHS_RXSTP)) {

        TRACE_DEBUG_WP("Stp ");

        // If a transfer was pending, complete it
        // Handle the case where during the status phase of a control write
        // transfer, the host receives the device ZLP and ack it, but the ack
        // is not received by the device
        if ((pEndpoint->dState == endpointStateWrite)
            || (pEndpoint->dState == endpointStateRead)) {

            OTGHS_EndOfTransfer(pEndpoint, USB_STATUS_SUCCESS);
        }

        // Copy the setup packet in S_usb
        OTGHS_GetSetup(pUsb);

        // Acknowledge setup packet
        pInterface->OTGHS_DEVEPTCCR[bEndpoint] = AT91C_OTGHS_RXSTP;

        // Forward the request to the upper layer
        USB_NewRequestCallback(pUsb);
    }

    // STALL sent
    if (ISSET(dStatus, AT91C_OTGHS_STALL)) {

        TRACE_WARNING("Sta 0x%X [%d] ", dStatus, bEndpoint);

        // Acknowledge STALL interrupt and disable it
        pInterface->OTGHS_DEVEPTCCR[bEndpoint] = AT91C_OTGHS_STALL;
        //pInterface->OTGHS_DEVEPTCDR[bEndpoint] = AT91C_OTGHS_STALL;

        // If the endpoint is not halted, clear the stall condition
        if (pEndpoint->dState != endpointStateHalted) {

            TRACE_WARNING("_ " );
            // Acknowledge the stall RQ flag
            pInterface->OTGHS_DEVEPTCDR[bEndpoint] = AT91C_OTGHS_STALLRQ;
        }

    }

}


//------------------------------------------------------------------------------
//      Exported functions
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// \brief  Configure an endpoint with the provided endpoint descriptor
// \param  pUsb    Pointer to a S_usb instance
// \param  pEpDesc Pointer to the endpoint descriptor
// \return true if the endpoint is now configured, false otherwise
// \see    S_usb_endpoint_descriptor
// \see    S_usb
//------------------------------------------------------------------------------
static bool OTGHS_ConfigureEndpoint(const S_usb *pUsb,
                             const S_usb_endpoint_descriptor *pEpDesc)
{
    AT91PS_OTGHS   pInterface = OTGHS_GetDriverInterface(pUsb);
    S_usb_endpoint *pEndpoint;
    unsigned char  bEndpoint;
    unsigned char  bType;
    unsigned char  endpointDir;
    unsigned short sizeEpt = 0;

    // Maximum packet size configuration value
    if( pEpDesc->wMaxPacketSize == 8 ) {
        sizeEpt = AT91C_OTGHS_EPT_SIZE_8;
    } else if ( pEpDesc->wMaxPacketSize == 16 ) {
        sizeEpt = AT91C_OTGHS_EPT_SIZE_16;
    } else if ( pEpDesc->wMaxPacketSize == 32 ) {
        sizeEpt = AT91C_OTGHS_EPT_SIZE_32;
    } else if ( pEpDesc->wMaxPacketSize == 64 ) {
        sizeEpt = AT91C_OTGHS_EPT_SIZE_64;
    } else if ( pEpDesc->wMaxPacketSize == 128 ) {
        sizeEpt = AT91C_OTGHS_EPT_SIZE_128;
    } else if ( pEpDesc->wMaxPacketSize == 256 ) {
        sizeEpt = AT91C_OTGHS_EPT_SIZE_256;
    } else if ( pEpDesc->wMaxPacketSize == 512 ) {
        sizeEpt = AT91C_OTGHS_EPT_SIZE_512;
    } else if ( pEpDesc->wMaxPacketSize == 1024 ) {
        sizeEpt = AT91C_OTGHS_EPT_SIZE_1024;
    } //else {
    //  sizeEpt = 0; // control endpoint
    //}

    // if pEpDesc == 0 then initialize the control endpoint
    if (pEpDesc == (S_usb_endpoint_descriptor const *) 0) {

        bEndpoint = 0;
        bType = 0;    // Control endpoint
    }
    else {
        // The endpoint number
        bEndpoint   = (unsigned char) (pEpDesc->bEndpointAddress & 0x7);
        // Transfer type: Control, Isochronous, Bulk, Interrupt
        bType = (unsigned char) (pEpDesc->bmAttributes     & 0x3);
        // Direction, ignored for control endpoints
        endpointDir  = (unsigned char) (pEpDesc->bEndpointAddress & (1<<7));
    }

    // Get pointer on endpoint
    pEndpoint = USB_GetEndpoint(pUsb, bEndpoint);
    if (pEndpoint == 0) {

        return false;
    }

    // Configure wMaxPacketSize
    if (pEpDesc != 0) {

        pEndpoint->wMaxPacketSize = pEpDesc->wMaxPacketSize;
    }
    else {

        pEndpoint->wMaxPacketSize = USB_ENDPOINT0_MAXPACKETSIZE;
    }

    // Abort the current transfer is the endpoint was configured and in
    // Write or Read state
    if ((pEndpoint->dState == endpointStateRead)
        || (pEndpoint->dState == endpointStateWrite)) {

        OTGHS_EndOfTransfer(pEndpoint, USB_STATUS_RESET);
    }

    // Enter in IDLE state
    pEndpoint->dState = endpointStateIdle;

    // Reset Endpoint Fifos
    pInterface->OTGHS_DEVEPT |= (1<<bEndpoint<<16);
    pInterface->OTGHS_DEVEPT &= ~(1<<bEndpoint<<16);

    // Enable endpoint
    pInterface->OTGHS_DEVEPT |= (1<<bEndpoint);

    // Configure endpoint
    switch (bType) {

        //-------------------------
        case ENDPOINT_TYPE_CONTROL:
        //-------------------------
            TRACE_INFO("Control[%d]\n\r",bEndpoint);

            //! Configure endpoint
            pInterface->OTGHS_DEVEPTCFG[bEndpoint] = AT91C_OTGHS_ALLOC |
                            AT91C_OTGHS_EPT_SIZE_64 | AT91C_OTGHS_EPT_DIR_OUT | AT91C_OTGHS_EPT_TYPE_CTL_EPT | AT91C_OTGHS_BK_NUMBER_1;

            // Enable RXSTP interrupt
            pInterface->OTGHS_DEVEPTCER[bEndpoint] = AT91C_OTGHS_RXSTP;

            // Enable endpoint IT
            pInterface->OTGHS_DEVIER = 1<<SHIFT_INTERUPT<<bEndpoint;

            break;

        //-----------------------------
        case ENDPOINT_TYPE_ISOCHRONOUS:
        //-----------------------------
            if (endpointDir) {
                TRACE_INFO("Iso In[%d]\n\r",bEndpoint);

                //! Configure endpoint
#ifndef DMA
                pInterface->OTGHS_DEVEPTCFG[bEndpoint] = AT91C_OTGHS_ALLOC |
                                              sizeEpt | AT91C_OTGHS_EPT_DIR_IN | AT91C_OTGHS_EPT_TYPE_ISO_EPT | AT91C_OTGHS_BK_NUMBER_2;
#else
                pInterface->OTGHS_DEVEPTCFG[bEndpoint] = AT91C_OTGHS_ALLOC | AT91C_OTGHS_AUTOSW |
                                              sizeEpt | AT91C_OTGHS_EPT_DIR_IN | AT91C_OTGHS_EPT_TYPE_ISO_EPT | AT91C_OTGHS_BK_NUMBER_2;
#endif

            }
            else {
                TRACE_INFO("Iso Out[%d]\n\r",bEndpoint);

                //! Configure endpoint
#ifndef DMA
                pInterface->OTGHS_DEVEPTCFG[bEndpoint] =  AT91C_OTGHS_ALLOC |
                                             sizeEpt | AT91C_OTGHS_EPT_DIR_OUT | AT91C_OTGHS_EPT_TYPE_ISO_EPT | AT91C_OTGHS_BK_NUMBER_2;
#else
                pInterface->OTGHS_DEVEPTCFG[bEndpoint] = AT91C_OTGHS_ALLOC | AT91C_OTGHS_AUTOSW |
                                             sizeEpt | AT91C_OTGHS_EPT_DIR_OUT | AT91C_OTGHS_EPT_TYPE_ISO_EPT | AT91C_OTGHS_BK_NUMBER_2;
#endif

            }
            break;

        //----------------------
        case ENDPOINT_TYPE_BULK:
        //----------------------
            if (endpointDir) {
                TRACE_INFO("Bulk In(%d)[%d] ",bEndpoint, pEpDesc->wMaxPacketSize);
                //! Configure endpoint
#ifndef DMA
                pInterface->OTGHS_DEVEPTCFG[bEndpoint] = AT91C_OTGHS_ALLOC |
                                              sizeEpt | AT91C_OTGHS_EPT_DIR_IN | AT91C_OTGHS_EPT_TYPE_BUL_EPT | AT91C_OTGHS_BK_NUMBER_2;
#else
                pInterface->OTGHS_DEVEPTCFG[bEndpoint] = AT91C_OTGHS_ALLOC | AT91C_OTGHS_AUTOSW |
                                              sizeEpt | AT91C_OTGHS_EPT_DIR_IN | AT91C_OTGHS_EPT_TYPE_BUL_EPT | AT91C_OTGHS_BK_NUMBER_2;
#endif

            }
            else {
                TRACE_INFO("Bulk Out(%d)[%d]\n\r",bEndpoint, pEpDesc->wMaxPacketSize);
                //! Configure endpoint
#ifndef DMA
                pInterface->OTGHS_DEVEPTCFG[bEndpoint] =  AT91C_OTGHS_ALLOC |
                                             sizeEpt | AT91C_OTGHS_EPT_DIR_OUT | AT91C_OTGHS_EPT_TYPE_BUL_EPT | AT91C_OTGHS_BK_NUMBER_2;
#else
                pInterface->OTGHS_DEVEPTCFG[bEndpoint] = AT91C_OTGHS_ALLOC | AT91C_OTGHS_AUTOSW |
                                             sizeEpt | AT91C_OTGHS_EPT_DIR_OUT | AT91C_OTGHS_EPT_TYPE_BUL_EPT | AT91C_OTGHS_BK_NUMBER_2;
#endif
            }
            break;

        //---------------------------
        case ENDPOINT_TYPE_INTERRUPT:
        //---------------------------
            if (endpointDir) {
                TRACE_INFO("Interrupt In[%d]\n\r",bEndpoint);
                //! Configure endpoint
#ifndef DMA
                pInterface->OTGHS_DEVEPTCFG[bEndpoint] = AT91C_OTGHS_ALLOC |
                                              sizeEpt | AT91C_OTGHS_EPT_DIR_IN | AT91C_OTGHS_EPT_TYPE_INT_EPT | AT91C_OTGHS_BK_NUMBER_2;
#else
                pInterface->OTGHS_DEVEPTCFG[bEndpoint] = AT91C_OTGHS_ALLOC | AT91C_OTGHS_AUTOSW |
                                              sizeEpt | AT91C_OTGHS_EPT_DIR_IN | AT91C_OTGHS_EPT_TYPE_INT_EPT | AT91C_OTGHS_BK_NUMBER_2;
#endif

            }
            else {
                TRACE_INFO("Interrupt Out[%d]\n\r",bEndpoint);
                //! Configure endpoint
#ifndef DMA
                pInterface->OTGHS_DEVEPTCFG[bEndpoint] =  AT91C_OTGHS_ALLOC |
                                             sizeEpt | AT91C_OTGHS_EPT_DIR_OUT | AT91C_OTGHS_EPT_TYPE_INT_EPT | AT91C_OTGHS_BK_NUMBER_2;
#else
                pInterface->OTGHS_DEVEPTCFG[bEndpoint] = AT91C_OTGHS_ALLOC | AT91C_OTGHS_AUTOSW |
                                             sizeEpt | AT91C_OTGHS_EPT_DIR_OUT | AT91C_OTGHS_EPT_TYPE_INT_EPT | AT91C_OTGHS_BK_NUMBER_2;
#endif

            }
            break;

        //------
        default:
        //------
            TRACE_ERROR(" unknown endpoint type\n\r");
            return false;
    }

    // Check if the configuration is ok
    if (ISCLEARED(pInterface->OTGHS_DEVEPTCSR[bEndpoint], AT91C_OTGHS_CFGOK)) {

        TRACE_FATAL("OTGHS_ConfigureEndpoint: Cannot configure endpoint\n\r");
        return false;
    }

    return true;
}


//------------------------------------------------------------------------------
//      Interrupt service routine
//------------------------------------------------------------------------------
#ifdef DMA
//----------------------------------------------------------------------------
//! \fn    OTGHS_DmaHandler
//! \brief This function (ISR) handles DMA interrupts
//----------------------------------------------------------------------------
static void OTGHS_DmaHandler(const S_usb *pUsb, unsigned char endpoint)
{
    AT91PS_OTGHS   pInterface = OTGHS_GetDriverInterface(pUsb);
    S_usb_endpoint *pEndpoint = USB_GetEndpoint(pUsb, endpoint);
    unsigned int   csr;

    csr = pInterface->OTGHS_DEVDMA[endpoint].OTGHS_DEVDMASTATUS;
    pInterface->OTGHS_DEVIDR = (1<<SHIFT_DMA<<endpoint);

    if((csr & AT91C_OTGHS_END_BF_ST) || (csr & AT91C_OTGHS_END_TR_ST)) {
        // READ
        TRACE_DEBUG_M("END_BF_ST\n\r");
        pEndpoint->dBytesTransferred = pEndpoint->dBytesBuffered;
        pEndpoint->dBytesBuffered = 0;

        TRACE_DEBUG_M("dBytesBuffered: 0x%x\n\r",pEndpoint->dBytesBuffered);
        TRACE_DEBUG_M("dBytesRemaining: 0x%x\n\r",pEndpoint->dBytesRemaining);
        TRACE_DEBUG_M("dBytesTransferred: 0x%x\n\r",pEndpoint->dBytesTransferred);

        OTGHS_EndOfTransfer(pEndpoint, USB_STATUS_SUCCESS);
        pEndpoint->dState = endpointStateIdle;
    }
    else {
        TRACE_FATAL("Probleme IT DMA\n\r");
    }
}
#endif


//------------------------------------------------------------------------------
// \brief  OTGHS interrupt handler
//
//         Manages device resume, suspend, end of bus reset. Forwards endpoint
//         interrupts to the appropriate handler.
// \param  pUsb Pointer to a S_usb instance
//------------------------------------------------------------------------------
static void OTGHS_Handler(const S_usb *pUsb)
{
    AT91PS_OTGHS  pInterface = OTGHS_GetDriverInterface(pUsb);
    unsigned int  dStatus;
    unsigned char numIT;

    if ( (!ISSET(USB_GetState(pUsb), USB_STATE_SUSPENDED))
       && (ISSET(USB_GetState(pUsb), USB_STATE_POWERED))){

        LED_TOGGLE(LED_USB);
    }

    TRACE_DEBUG_H("Hlr ");

    // Get General interrupts status
    dStatus = pInterface->OTGHS_SR & pInterface->OTGHS_CTRL & 0xFF;
    while (dStatus != 0) {

        if(ISSET(dStatus, AT91C_OTGHS_VBUSTI))
        {
            TRACE_DEBUG_M("__VBus\n\r");

            USB_Attach(pUsb);

            // Acknowledge the interrupt
            pInterface->OTGHS_SCR = AT91C_OTGHS_VBUSTI;
        }

        // Don't treat others interrupt for this time
        pInterface->OTGHS_SCR = AT91C_OTGHS_IDT    | AT91C_OTGHS_SRP 
                              | AT91C_OTGHS_VBERR  | AT91C_OTGHS_BCERR
                              | AT91C_OTGHS_ROLEEX | AT91C_OTGHS_HNPERR
                              | AT91C_OTGHS_STO;

        dStatus = pInterface->OTGHS_SR & pInterface->OTGHS_CTRL & 0xFF;
    }


    // Get OTG Device interrupts status
    dStatus = pInterface->OTGHS_DEVISR & pInterface->OTGHS_DEVIMR;
    TRACE_DEBUG_H("OTGHS_DEVISR:0x%X\n\r", pInterface->OTGHS_DEVISR);
    while (dStatus != 0) {

        // Start Of Frame (SOF)
        if (ISSET(dStatus, AT91C_OTGHS_SOF)) {
            TRACE_DEBUG_WP("SOF ");

            // Invoke the SOF callback
            USB_StartOfFrameCallback(pUsb);

            // Acknowledge interrupt
            SET(pInterface->OTGHS_DEVICR, AT91C_OTGHS_SOF);
            CLEAR(dStatus, AT91C_OTGHS_SOF);
        }

        // Suspend
        else if (dStatus & AT91C_OTGHS_SUSP) {

            TRACE_DEBUG_M("S ");

            if (!ISSET(USB_GetState(pUsb), USB_STATE_SUSPENDED)) {

                // The device enters the Suspended state
                //      MCK + UDPCK must be off
                //      Pull-Up must be connected
                //      Transceiver must be disabled

                // Enable wakeup
                SET(pInterface->OTGHS_DEVIER, AT91C_OTGHS_EORST | AT91C_OTGHS_WAKEUP | AT91C_OTGHS_EORSM);

                // Acknowledge interrupt
                pInterface->OTGHS_DEVICR = AT91C_OTGHS_SUSP;
                SET(*(pUsb->pState), USB_STATE_SUSPENDED);
                OTGHS_DisableTransceiver(pUsb);
                OTGHS_DisableMCK(pUsb);
                OTGHS_DisableOTGHSCK(pUsb);

                // Invoke the Suspend callback

                USB_SuspendCallback(pUsb);
            }
        }

        // Resume
        else if (ISSET(dStatus, AT91C_OTGHS_WAKEUP)
              || ISSET(dStatus, AT91C_OTGHS_EORSM)) {

            // Invoke the Resume callback
            USB_ResumeCallback(pUsb);

            TRACE_DEBUG_M("R ");

            // The device enters Configured state
            //      MCK + UDPCK must be on
            //      Pull-Up must be connected
            //      Transceiver must be enabled

            if (ISSET(USB_GetState(pUsb), USB_STATE_SUSPENDED)) {

                // Powered state
                OTGHS_EnableMCK(pUsb);
                OTGHS_EnableOTGHSCK(pUsb);

                // Default state
                if (ISSET(USB_GetState(pUsb), USB_STATE_DEFAULT)) {

                    OTGHS_EnableTransceiver(pUsb);
                }

                CLEAR(*(pUsb->pState), USB_STATE_SUSPENDED);
            }
            pInterface->OTGHS_DEVICR = 
                (AT91C_OTGHS_WAKEUP | AT91C_OTGHS_EORSM | AT91C_OTGHS_SUSP);

            pInterface->OTGHS_DEVIER = (AT91C_OTGHS_EORST | AT91C_OTGHS_SUSP);
            pInterface->OTGHS_DEVICR = (AT91C_OTGHS_WAKEUP | AT91C_OTGHS_EORSM);
            pInterface->OTGHS_DEVIDR = AT91C_OTGHS_WAKEUP;

        }

        // End of bus reset
        else if (dStatus & AT91C_OTGHS_EORST) {

            TRACE_DEBUG_M("EoB ");
            // The device enters the Default state
            //      MCK + UDPCK are already enabled
            //      Pull-Up is already connected
            //      Transceiver must be enabled
            //      Endpoint 0 must be enabled
            SET(*(pUsb->pState), USB_STATE_DEFAULT);

            OTGHS_EnableTransceiver(pUsb);

            // The device leaves the Address & Configured states
            CLEAR(*(pUsb->pState), USB_STATE_ADDRESS | USB_STATE_CONFIGURED);
            OTGHS_ResetEndpoints(pUsb);
            OTGHS_DisableEndpoints(pUsb);
            OTGHS_ConfigureEndpoint(pUsb, 0);

            // Flush and enable the Suspend interrupt
            SET(pInterface->OTGHS_DEVICR, AT91C_OTGHS_WAKEUP | AT91C_OTGHS_SUSP);

            // Enable the Start Of Frame (SOF) interrupt if needed
            if (pUsb->pCallbacks->startOfFrame != 0) {

                SET(pInterface->OTGHS_DEVIER, AT91C_OTGHS_SOF);
            }

            // Invoke the Reset callback
            USB_ResetCallback(pUsb);

            // Acknowledge end of bus reset interrupt
            pInterface->OTGHS_DEVICR = AT91C_OTGHS_EORST;
        }

        // Handle upstream resume interrupt
        else if (dStatus & AT91C_OTGHS_UPRSM) {

            TRACE_DEBUG_WP("  External resume interrupt\n\r");

            // - Acknowledge the IT
            pInterface->OTGHS_DEVICR = AT91C_OTGHS_UPRSM;
        }

        // Endpoint interrupts
        else {
#ifndef DMA
            // Handle endpoint interrupts
            for (numIT = 0; numIT < NUM_IT_MAX; numIT++) {
                if( dStatus & (1<<SHIFT_INTERUPT<<numIT) ) {
                    OTGHS_EndpointHandler(pUsb, numIT);
                }
            }
#else
            // Handle endpoint control interrupt
            if( dStatus & (1<<SHIFT_INTERUPT<<0) ) {
                OTGHS_EndpointHandler(pUsb, 0);
            }
            // Handle DMA interrupts
            for(numIT = 1; numIT <= NUM_IT_MAX_DMA; numIT++) {
                if( dStatus & (1<<SHIFT_DMA<<numIT) ) {
                    OTGHS_DmaHandler(pUsb, numIT);
                }
            }
#endif
        }

        // Retrieve new interrupt status
        dStatus = (pInterface->OTGHS_DEVISR) & (pInterface->OTGHS_DEVIMR);

        // Mask unneeded interrupts
        if (!ISSET(USB_GetState(pUsb), USB_STATE_DEFAULT)) {

            dStatus &= AT91C_OTGHS_EORST | AT91C_OTGHS_SOF;
        }

        TRACE_DEBUG_H("\n\r");

        if (dStatus != 0) {

            TRACE_DEBUG_WP("  - ");
        }
    }

    if ( (!ISSET(USB_GetState(pUsb), USB_STATE_SUSPENDED))
       && (ISSET(USB_GetState(pUsb), USB_STATE_POWERED))){

        LED_TOGGLE(LED_USB);
    }
}

//------------------------------------------------------------------------------
// \brief  Sends data through an USB endpoint
//
//         Sets up the transfer descriptor, write one or two data payloads
//         (depending on the number of FIFO banks for the endpoint) and then
//         starts the actual transfer. The operation is complete when all
//         the data has been sent.
// \param  pUsb      Pointer to a S_usb instance
// \param  bEndpoint Index of endpoint
// \param  pData     Pointer to a buffer containing the data to send
// \param  dLength   Length of the data buffer
// \param  fCallback Optional function to invoke when the transfer finishes
// \param  pArgument Optional argument for the callback function
// \return Operation result code
// \see    Operation result codes
// \see    Callback_f
// \see    S_usb
//------------------------------------------------------------------------------
static char OTGHS_Write(const S_usb   *pUsb,
                        unsigned char bEndpoint,
                        const void    *pData,
                        unsigned int  dLength,
                        Callback_f    fCallback,
                        void          *pArgument)
{
    S_usb_endpoint *pEndpoint = USB_GetEndpoint(pUsb, bEndpoint);
    AT91PS_OTGHS pInterface = OTGHS_GetDriverInterface(pUsb);

    // Check that the endpoint is in Idle state
    if (pEndpoint->dState != endpointStateIdle) {

        return USB_STATUS_LOCKED;
    }

    TRACE_DEBUG_WP("Write%d(%d) ", bEndpoint, dLength);

    // Setup the transfer descriptor
    pEndpoint->pData = (char *) pData;
    pEndpoint->dBytesRemaining = dLength;
    pEndpoint->dBytesBuffered = 0;
    pEndpoint->dBytesTransferred = 0;
    pEndpoint->fCallback = fCallback;
    pEndpoint->pArgument = pArgument;
    pEndpoint->isDataSent = false;
    
    // Send one packet
    pEndpoint->dState = endpointStateWrite;

#ifdef DMA
    // Test if endpoint type control
    if (AT91C_OTGHS_EPT_TYPE_CTL_EPT == (AT91C_OTGHS_EPT_TYPE & pInterface->OTGHS_DEVEPTCFG[bEndpoint])) {
#endif
        // Enable endpoint IT
        pInterface->OTGHS_DEVIER = (1<<SHIFT_INTERUPT<<bEndpoint);
        pInterface->OTGHS_DEVEPTCER[bEndpoint] = AT91C_OTGHS_TXINI;

#ifdef DMA
    }
    else {

        // others endoint (not control)
        pEndpoint->dBytesBuffered = pEndpoint->dBytesRemaining;
        pEndpoint->dBytesRemaining = 0;

        pInterface->OTGHS_DEVDMA[bEndpoint].OTGHS_DEVDMAADDRESS = (unsigned int) pEndpoint->pData;

        // Enable IT DMA
        pInterface->OTGHS_DEVIER = (1<<SHIFT_DMA<<bEndpoint);

        pInterface->OTGHS_DEVDMA[bEndpoint].OTGHS_DEVDMACONTROL = 
             (((pEndpoint->dBytesBuffered<<16)&AT91C_OTGHS_BUFF_LENGTH)
               | AT91C_OTGHS_END_B_EN
               | AT91C_OTGHS_END_BUFFIT
               | AT91C_OTGHS_CHANN_ENB);

    }
#endif

    return USB_STATUS_SUCCESS;
}

//------------------------------------------------------------------------------
// \brief  Reads incoming data on an USB endpoint
//
//         This methods sets the transfer descriptor and activate the endpoint
//         interrupt. The actual transfer is then carried out by the endpoint
//         interrupt handler. The Read operation finishes either when the
//         buffer is full, or a short packet (inferior to endpoint maximum
//         packet size) is received.
// \param  pUsb      Pointer to a S_usb instance
// \param  bEndpoint Index of endpoint
// \param  pData     Pointer to a buffer to store the received data
// \param  dLength   Length of the receive buffer
// \param  fCallback Optional callback function
// \param  pArgument Optional callback argument
// \return Operation result code
// \see    Callback_f
// \see    S_usb
//------------------------------------------------------------------------------
static char OTGHS_Read(const S_usb   *pUsb,
                       unsigned char bEndpoint,
                       void          *pData,
                       unsigned int  dLength,
                       Callback_f    fCallback,
                       void          *pArgument)
{
    AT91PS_OTGHS pInterface = OTGHS_GetDriverInterface(pUsb);
    S_usb_endpoint *pEndpoint = USB_GetEndpoint(pUsb, bEndpoint);

    //! Return if the endpoint is not in IDLE state
    if (pEndpoint->dState != endpointStateIdle) {

        return USB_STATUS_LOCKED;
    }

    TRACE_DEBUG_M("Read%d(%d) ", bEndpoint, dLength);

    // Endpoint enters Read state
    pEndpoint->dState = endpointStateRead;

    //! Set the transfer descriptor
    pEndpoint->pData = (char *) pData;
    pEndpoint->dBytesRemaining = dLength;
    pEndpoint->dBytesBuffered = 0;
    pEndpoint->dBytesTransferred = 0;
    pEndpoint->fCallback = fCallback;
    pEndpoint->pArgument = pArgument;

#ifdef DMA
    // Test if endpoint type control
    if (AT91C_OTGHS_EPT_TYPE_CTL_EPT == (AT91C_OTGHS_EPT_TYPE & pInterface->OTGHS_DEVEPTCFG[bEndpoint])) {
#endif
        // Control endpoint
        // Enable endpoint IT
        pInterface->OTGHS_DEVIER = (1<<SHIFT_INTERUPT<<bEndpoint);
        pInterface->OTGHS_DEVEPTCER[bEndpoint] = AT91C_OTGHS_RXOUT;
#ifdef DMA
    }
    else {

        // others endoint (not control)
        pEndpoint->dBytesBuffered = pEndpoint->dBytesRemaining;
        pEndpoint->dBytesRemaining = 0;

        // Enable IT DMA
        pInterface->OTGHS_DEVIER = (1<<SHIFT_DMA<<bEndpoint);

        pInterface->OTGHS_DEVDMA[bEndpoint].OTGHS_DEVDMAADDRESS = (unsigned int) pEndpoint->pData;

        pInterface->OTGHS_DEVDMA[bEndpoint].OTGHS_DEVDMACONTROL = \
                             ( (pEndpoint->dBytesBuffered<<16)
                               | AT91C_OTGHS_END_TR_EN
                               | AT91C_OTGHS_END_TR_IT
                               | AT91C_OTGHS_END_B_EN
                               | AT91C_OTGHS_END_BUFFIT
                               | AT91C_OTGHS_CHANN_ENB);
    }
#endif

  return USB_STATUS_SUCCESS;
}

//------------------------------------------------------------------------------
// \brief  Clears, sets or returns the Halt state on specified endpoint
//
//         When in Halt state, an endpoint acknowledges every received packet
//         with a STALL handshake. This continues until the endpoint is
//         manually put out of the Halt state by calling this function.
// \param  pUsb Pointer to a S_usb instance
// \param  bEndpoint Index of endpoint
// \param  bRequest  Request to perform
//                   -> USB_SET_FEATURE, USB_CLEAR_FEATURE, USB_GET_STATUS
// \return true if the endpoint is currently Halted, false otherwise
// \see    S_usb
//------------------------------------------------------------------------------
static bool OTGHS_Halt(const S_usb   *pUsb,
                       unsigned char bEndpoint,
                       unsigned char bRequest)
{
    AT91PS_OTGHS pInterface = OTGHS_GetDriverInterface(pUsb);
    S_usb_endpoint *pEndpoint = USB_GetEndpoint(pUsb, bEndpoint);

    // Clear the Halt feature of the endpoint if it is enabled
    if (bRequest == USB_CLEAR_FEATURE) {

        TRACE_DEBUG_WP("Unhalt%d ", bEndpoint);

        // Return endpoint to Idle state
        pEndpoint->dState = endpointStateIdle;

        // Clear FORCESTALL flag

        // Disable stall on endpoint
        pInterface->OTGHS_DEVEPTCDR[bEndpoint] = AT91C_OTGHS_STALLRQ;
        pEndpoint->dState = endpointStateIdle;

        // Reset data-toggle
        pInterface->OTGHS_DEVEPTCER[bEndpoint] = AT91C_OTGHS_RSTDT;
    }
    // Set the Halt feature on the endpoint if it is not already enabled
    // and the endpoint is not disabled
    else if ((bRequest == USB_SET_FEATURE)
             && (pEndpoint->dState != endpointStateHalted)
             && (pEndpoint->dState != endpointStateDisabled)) {

        TRACE_DEBUG_WP("Halt%d ", bEndpoint);

        // Abort the current transfer if necessary
        OTGHS_EndOfTransfer(pEndpoint, USB_STATUS_ABORTED);

        // Put endpoint into Halt state
        pInterface->OTGHS_DEVEPTCER[bEndpoint] = AT91C_OTGHS_STALLRQ;
        pEndpoint->dState = endpointStateHalted;

        // Enable the endpoint interrupt
        pInterface->OTGHS_DEVIER = (1<<SHIFT_INTERUPT<<bEndpoint);
    }

    // Return the endpoint halt status
    if (pEndpoint->dState == endpointStateHalted) {

        return true;
    }
    else {

        return false;
    }
}

//------------------------------------------------------------------------------
// \brief  Causes the endpoint to acknowledge the next received packet with
//         a STALL handshake.
//
//         Further packets are then handled normally.
// \param  pUsb      Pointer to a S_usb instance
// \param  bEndpoint Index of endpoint
// \return Operation result code
// \see    S_usb
//------------------------------------------------------------------------------
static char OTGHS_Stall(const S_usb *pUsb,
                        unsigned char bEndpoint)
{
    AT91PS_OTGHS pInterface = OTGHS_GetDriverInterface(pUsb);
    S_usb_endpoint *pEndpoint = USB_GetEndpoint(pUsb, bEndpoint);

    // Check that endpoint is in Idle state
    if (pEndpoint->dState != endpointStateIdle) {

        TRACE_WARNING("UDP_Stall: Endpoint%d locked\n\r", bEndpoint);
        return USB_STATUS_LOCKED;
    }

    TRACE_DEBUG_WP("Stall%d ", bEndpoint);

    pInterface->OTGHS_DEVEPTCER[bEndpoint] = AT91C_OTGHS_STALL;
    pInterface->OTGHS_DEVEPTCER[bEndpoint] = AT91C_OTGHS_STALLRQ;

    return USB_STATUS_SUCCESS;
}

//------------------------------------------------------------------------------
// \brief  Activates a remote wakeup procedure
// \param  pUsb Pointer to a S_usb instance
// \see    S_usb
//------------------------------------------------------------------------------
static void OTGHS_RemoteWakeUp(const S_usb *pUsb)
{
    AT91PS_OTGHS pInterface = OTGHS_GetDriverInterface(pUsb);

    OTGHS_EnableMCK(pUsb);
    OTGHS_EnableOTGHSCK(pUsb);
    OTGHS_EnableTransceiver(pUsb);

    TRACE_DEBUG_WP("Remote WakeUp ");

    //! Enable wakeup interrupt
    //pInterface->OTGHS_DEVIER = AT91C_OTGHS_UPRSM;

    // Activates a remote wakeup
    pInterface->OTGHS_DEVCTRL |= AT91C_OTGHS_RMWKUP;
}

//------------------------------------------------------------------------------
// \brief  Handles attachment or detachment from the USB when the VBus power
//         line status changes.
// \param  pUsb Pointer to a S_usb instance
// \return true if VBus is present, false otherwise
// \see    S_usb
//------------------------------------------------------------------------------
static bool OTGHS_Attach(const S_usb *pUsb)
{
    AT91PS_OTGHS pInterface = OTGHS_GetDriverInterface(pUsb);

    TRACE_DEBUG_WP("Attach(");

    // Check if VBus is present
    if (!ISSET(USB_GetState(pUsb), USB_STATE_POWERED)
        && BRD_IsVBusConnected(pInterface)) {

        // Powered state:
        //      MCK + UDPCK must be on
        //      Pull-Up must be connected
        //      Transceiver must be disabled

        // Invoke the Resume callback
        USB_ResumeCallback(pUsb);

        OTGHS_EnableMCK(pUsb);
        OTGHS_EnableOTGHSCK(pUsb);

        // Enable the transceiver
        OTGHS_EnableTransceiver(pUsb);

        // Reconnect the pull-up if needed
        if (ISSET(*(pUsb->pState), USB_STATE_SHOULD_RECONNECT)) {

            USB_Connect(pUsb);
            CLEAR(*(pUsb->pState), USB_STATE_SHOULD_RECONNECT);
        }

        // Clear the Suspend and Resume interrupts
        pInterface->OTGHS_DEVICR = \
             AT91C_OTGHS_WAKEUP | AT91C_OTGHS_EORSM | AT91C_OTGHS_SUSP;

        // Enable interrupt
        pInterface->OTGHS_DEVIER = AT91C_OTGHS_EORST | AT91C_OTGHS_WAKEUP | AT91C_OTGHS_EORSM;
    
        // The device is in Powered state
        SET(*(pUsb->pState), USB_STATE_POWERED);

    }
    else if (ISSET(USB_GetState(pUsb), USB_STATE_POWERED)
             && !BRD_IsVBusConnected(pInterface)) {

        // Attached state:
        //      MCK + UDPCK off
        //      Pull-Up must be disconnected
        //      Transceiver must be disabled

        // Warning: MCK must be enabled to be able to write in UDP registers
        // It may have been disabled by the Suspend interrupt, so re-enable it
        OTGHS_EnableMCK(pUsb);

        // Disable interrupts
        pInterface->OTGHS_DEVIDR &= ~(AT91C_OTGHS_WAKEUP | AT91C_OTGHS_EORSM
                                    | AT91C_OTGHS_SUSP   | AT91C_OTGHS_SOF);

        OTGHS_DisableEndpoints(pUsb);

        // Disconnect the pull-up if needed
        if (ISSET(USB_GetState(pUsb), USB_STATE_DEFAULT)) {

            USB_Disconnect(pUsb);
            SET(*(pUsb->pState), USB_STATE_SHOULD_RECONNECT);
        }

        OTGHS_DisableTransceiver(pUsb);
        OTGHS_DisableMCK(pUsb);
        OTGHS_DisableOTGHSCK(pUsb);

        // The device leaves the all states except Attached
        CLEAR(*(pUsb->pState), USB_STATE_POWERED | USB_STATE_DEFAULT
              | USB_STATE_ADDRESS | USB_STATE_CONFIGURED | USB_STATE_SUSPENDED);

        // Invoke the Suspend callback
        USB_SuspendCallback(pUsb);

    }

    TRACE_DEBUG_WP("%d) ", ISSET(USB_GetState(pUsb), USB_STATE_POWERED));

    return ISSET(USB_GetState(pUsb), USB_STATE_POWERED);
}

//------------------------------------------------------------------------------
// \brief  Sets the device address
//
//         This function directly accesses the S_usb_request instance located
//         in the S_usb structure to extract its new address.
// \param  pUsb Pointer to a S_usb instance
// \see    S_usb
//------------------------------------------------------------------------------
static void OTGHS_SetAddress(S_usb const *pUsb)
{
    unsigned short wAddress = USB_GetSetup(pUsb)->wValue;
    AT91PS_OTGHS pInterface = OTGHS_GetDriverInterface(pUsb);

    TRACE_DEBUG_WP("SetAddr(%d) ", wAddress);

    // Set address
    pInterface->OTGHS_DEVCTRL = wAddress & AT91C_OTGHS_UADD;
    pInterface->OTGHS_DEVCTRL |= AT91C_OTGHS_ADDEN;

}

//------------------------------------------------------------------------------
// \brief  Changes the device state from Address to Configured, or from
//         Configured to Address.
//
//         This method directly access the last received SETUP packet to
//         decide on what to do.
// \see    S_usb
//------------------------------------------------------------------------------
static void OTGHS_SetConfiguration(S_usb const *pUsb)
{
    unsigned short wValue = USB_GetSetup(pUsb)->wValue;

    TRACE_DEBUG_WP("SetCfg() ");

    // Check the request
    if (wValue != 0) {
        // Enter Configured state
        SET(*(pUsb->pState), USB_STATE_CONFIGURED);

    }
    else {

        // Go back to Address state
        CLEAR(*(pUsb->pState), USB_STATE_CONFIGURED);

        // Abort all transfers
        OTGHS_DisableEndpoints(pUsb);
    }
}

//------------------------------------------------------------------------------
// \brief  Enables the pull-up on the D+ line to connect the device to the USB.
// \param  pUsb Pointer to a S_usb instance
// \see    S_usb
//------------------------------------------------------------------------------
static void OTGHS_Connect(const S_usb *pUsb)
{
#if defined(INTERNAL_PULLUP)
    CLEAR(OTGHS_GetDriverInterface(pUsb)->OTGHS_DEVCTRL, AT91C_OTGHS_DETACH);

#elif defined(INTERNAL_PULLUP_MATRIX)
    TRACE_DEBUG_WP("PUON 1\n\r");
    AT91C_BASE_MATRIX->MATRIX_USBPCR |= AT91C_MATRIX_USBPCR_PUON;

#else
    BRD_ConnectPullUp(UDP_GetDriverInterface(pUsb));

#endif
}

//------------------------------------------------------------------------------
// \brief  Disables the pull-up on the D+ line to disconnect the device from
//         the bus.
// \param  pUsb Pointer to a S_usb instance
// \see    S_usb
//------------------------------------------------------------------------------
static void OTGHS_Disconnect(const S_usb *pUsb)
{
#if defined(INTERNAL_PULLUP)
    SET(OTGHS_GetDriverInterface(pUsb)->OTGHS_DEVCTRL, AT91C_OTGHS_DETACH);

#elif defined(INTERNAL_PULLUP_MATRIX)
    TRACE_DEBUG_WP("PUON 0\n\r");
    AT91C_BASE_MATRIX->MATRIX_USBPCR &= ~AT91C_MATRIX_USBPCR_PUON;

#else
    BRD_DisconnectPullUp(UDP_GetDriverInterface(pUsb));

#endif
    // Device leaves the Default state
    CLEAR(*(pUsb->pState), USB_STATE_DEFAULT);
}

//------------------------------------------------------------------------------
// \brief  Certification test for High Speed device.
// \param  pUsb Pointer to a S_usb instance
// \param  bIndex char for the test choice
// \see    S_usb
//------------------------------------------------------------------------------
static void OTGHS_Test(const S_usb *pUsb, unsigned char bIndex)
{
    AT91PS_OTGHS pInterface = OTGHS_GetDriverInterface(pUsb);

    pInterface->OTGHS_DEVIDR &= ~AT91C_OTGHS_SUSP;
    pInterface->OTGHS_DEVCTRL |= AT91C_OTGHS_SPDCONF_HS; // remove suspend ?

    switch( bIndex ) {
        case TEST_PACKET:
            TRACE_DEBUG_M("TEST_PACKET ");
            pInterface->OTGHS_DEVCTRL |= AT91C_OTGHS_TSTPCKT;
            break;

        case TEST_J:
            TRACE_DEBUG_M("TEST_J ");
            pInterface->OTGHS_DEVCTRL |= AT91C_OTGHS_TSTJ;
            break;

        case TEST_K:
            TRACE_DEBUG_M("TEST_K ");
            pInterface->OTGHS_DEVCTRL |= AT91C_OTGHS_TSTK;
            break;

        case TEST_SEO_NAK:
            TRACE_DEBUG_M("TEST_SEO_NAK ");
            pInterface->OTGHS_DEVIDR = 0xFFFFFFFF;
            break;

        case TEST_SEND_ZLP:
            pInterface->OTGHS_DEVEPTCCR[0] = AT91C_OTGHS_TXINI;
            TRACE_DEBUG_M("SEND_ZLP ");
            break;

        TRACE_DEBUG_M("\n\r");
    }
}

//------------------------------------------------------------------------------
// \brief  Certification test for High Speed device.
// \param  pUsb Pointer to a S_usb instance
// \see    S_usb
//------------------------------------------------------------------------------
static bool OTGHS_IsHighSpeed(const S_usb *pUsb)
{
    AT91PS_OTGHS pInterface = OTGHS_GetDriverInterface(pUsb);
    bool         status = false;

    if(AT91C_OTGHS_SPEED_SR_HS == (pInterface->OTGHS_SR & (0x03<<12))) {
        // High Speed
        status = true;
    }

    return status;
}

//------------------------------------------------------------------------------
// \brief  Initializes the specified USB driver
//
//         This function initializes the current FIFO bank of endpoints,
//         configures the pull-up and VBus lines, disconnects the pull-up and
//         then trigger the Init callback.
// \param  pUsb Pointer to a S_usb instance
// \see    S_usb
//------------------------------------------------------------------------------
static void OTGHS_Init(const S_usb *pUsb)
{
    AT91PS_OTGHS  pInterface = OTGHS_GetDriverInterface(pUsb);
    unsigned char i;

    TRACE_DEBUG_WP("Init()\n\r");

    // Enable USB macro
    SET(OTGHS_GetDriverInterface(pUsb)->OTGHS_CTRL, AT91C_OTGHS_USBECTRL);

    pInterface->OTGHS_DEVCTRL &=~ AT91C_OTGHS_DETACH; // detach

    //// Force FS (for debug or test)
//    pDriver->OTGHS_DEVCTRL |= AT91C_OTGHS_SPDCONF_FS;
    pInterface->OTGHS_DEVCTRL &= ~AT91C_OTGHS_SPDCONF_FS;   // Normal mode
    pInterface->OTGHS_DEVCTRL &= ~(  AT91C_OTGHS_LS | AT91C_OTGHS_TSTJ
                                | AT91C_OTGHS_TSTK | AT91C_OTGHS_TSTPCKT
                                | AT91C_OTGHS_OPMODE2 ); // Normal mode


    // With OR without DMA !!!
    // Initialization of DMA
    for( i=1; i<=((AT91C_BASE_OTGHS->OTGHS_IPFEATURES & AT91C_OTGHS_DMA_CHANNEL_NBR)>>4); i++ ) {

        // RESET endpoint canal DMA:
        // DMA stop channel command
        AT91C_BASE_OTGHS->OTGHS_DEVDMA[i].OTGHS_DEVDMACONTROL = 0;  // STOP command

        // Disable endpoint
        AT91C_BASE_OTGHS->OTGHS_DEVEPTCDR[i] = 0XFFFFFFFF;

        // Reset endpoint config
        AT91C_BASE_OTGHS->OTGHS_DEVEPTCFG[i] = 0;

        // Reset DMA channel (Buff count and Control field)
        AT91C_BASE_OTGHS->OTGHS_DEVDMA[i].OTGHS_DEVDMACONTROL = 0x02;  // NON STOP command

        // Reset DMA channel 0 (STOP)
        AT91C_BASE_OTGHS->OTGHS_DEVDMA[i].OTGHS_DEVDMACONTROL = 0;  // STOP command

        // Clear DMA channel status (read the register for clear it)
        AT91C_BASE_OTGHS->OTGHS_DEVDMA[i].OTGHS_DEVDMASTATUS = AT91C_BASE_OTGHS->OTGHS_DEVDMA[i].OTGHS_DEVDMASTATUS;

    }

    // Enable clock OTG pad
    pInterface->OTGHS_CTRL &= ~AT91C_OTGHS_FRZCLKCTRL;

    // Clear General IT
    pInterface->OTGHS_SCR = 0x01FF;

    // Clear OTG Device IT
    pInterface->OTGHS_DEVICR = 0xFF;

    // Clear OTG Host IT
    pInterface->OTGHS_HSTICR = 0x7F;

    // Reset all Endpoints Fifos
    pInterface->OTGHS_DEVEPT |= (0x7F<<16);
    pInterface->OTGHS_DEVEPT &= ~(0x7F<<16);

    // Disable all endpoints
    pInterface->OTGHS_DEVEPT &= ~0x7F;

    // Bypass UTMI problems // jcb to be removed with new version of UTMI
    // pInterface->OTGHS_TSTA2 = (1<<6)|(1<<7)|(1<<8);
    // pInterface->OTGHS_TSTA2 = (1<<8);
    pInterface->OTGHS_TSTA2 = 0;

    // External pull-up on D+
    // Configure
    BRD_ConfigurePullUp(pInterface);

    // Detach
    OTGHS_Disconnect(pUsb);

    // Device is in the Attached state
    *(pUsb->pState) = USB_STATE_ATTACHED;

    // Disable the UDP transceiver and interrupts
    OTGHS_EnableMCK(pUsb);
    SET(pInterface->OTGHS_DEVIER, AT91C_OTGHS_EORSM);

    OTGHS_DisableMCK(pUsb);
    OTGHS_Disconnect(pUsb);

    // Test ID
    if( 0 != (pInterface->OTGHS_SR & AT91C_OTGHS_ID) ) {
        TRACE_INFO("ID=1: PERIPHERAL\n\r");
    }
    else {
        TRACE_INFO("ID=0: HOST\n\r");
    }

    // Test VBUS
    if( 0 != (pInterface->OTGHS_SR & AT91C_OTGHS_VBUSSR) ) {
        TRACE_INFO("VBUS = 1\n\r");
    }
    else {
        TRACE_INFO("VBUS = 0\n\r");
    }

    // Test SPEED
    if(AT91C_OTGHS_SPEED_SR_HS == (pInterface->OTGHS_SR & (0x03<<12))) {
        TRACE_INFO("HIGH SPEED\n\r");
    }
    else if(AT91C_OTGHS_SPEED_SR_LS == (pInterface->OTGHS_SR & (0x03<<12))) {
        TRACE_INFO("LOW SPEED\n\r");
    }
    else {
        TRACE_INFO("FULL SPEED\n\r");
    }

    // Configure interrupts
    USB_InitCallback(pUsb);

    pInterface->OTGHS_CTRL |= AT91C_OTGHS_VBUSTI;
}

//------------------------------------------------------------------------------
//      Global variables
//------------------------------------------------------------------------------

// \brief Low-level driver methods to use with the OTGHS USB controller
// \see S_driver_methods
const S_driver_methods sOTGHSMethods = {

    OTGHS_Init,
    OTGHS_Write,
    OTGHS_Read,
    OTGHS_Stall,
    OTGHS_Halt,
    OTGHS_RemoteWakeUp,
    OTGHS_ConfigureEndpoint,
    OTGHS_Attach,
    OTGHS_SetAddress,
    OTGHS_SetConfiguration,
    OTGHS_Handler,
    OTGHS_Connect,
    OTGHS_Disconnect,
    OTGHS_Test,
    OTGHS_IsHighSpeed
};

// \brief  Default driver when an UDP controller is present on a chip
const S_usb_driver sDefaultDriver = {

    AT91C_BASE_OTGHS,
    AT91C_BASE_OTGHS_EPTFIFO,
    0,
    AT91C_ID_OTGHS,
    AT91C_PMC_OTG,
    &sOTGHSMethods
};

#endif //#ifdef CHIP_OTGHS

