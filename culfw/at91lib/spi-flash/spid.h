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
/// The Spi driver is a low level spi driver which performs SPI device Initializes, 
/// spi transfer and receive. It can be used by upper SPI driver such as AT45 
/// driver and AT26 driver. 
///  
/// !!!Usage
/// 
/// -# Initializes a SPI instance and the corresponding SPI hardware,
///    Configure SPI in Master Mode using SPID_Configure().
/// -# Configures the SPI characteristics (such as Clock Polarity, Phase, 
///    transfers delay and Baud Rate) for the device corresponding to the
///    chip select using SPID_ConfigureCS().
/// -# Starts a SPI master transfer using SPID_SendCommand().
///    The transfer is performed using the PDC channels. 
///    -# It enable the SPI clock.
///    -# Set the corresponding peripheral chip select.
///    -# Initialize the two SPI PDC buffers.
///       - Initialize SPI_TPR and SPI_TCR with SPI command data and size
///         to send command data first.
///       - Initialize SPI_RPR and SPI_RCR with SPI command data and size
///         as dummy value.
///       - Initialize SPI_TNPR and SPI_TNCR with rest of the data to be 
///        transfered.(if the data specified in cmd structure)
///       - Initialize SPI_RNPR and SPI_RNCR with rest of the data to be 
///         received.(if the data specified in cmd structure)
///    -# Initialize the callback function if specified.
///    -# Enable transmitter and receiver.
///    -# Example for sending a command to the dataflash through the SPI. 
/// \code
///      /// Build command to be sent.
///      ...
///      // Send Command and data through the SPI
///      if (SPID_SendCommand(pAt45->pSpid, pCommand)) {
///          return AT45_ERROR_SPI;
///      }
/// \endcode
/// -# The SPI_Handler() must be called by the SPI Interrupt Service Routine 
///    with the corresponding Spi instance. It is invokes to check for pending
///    interrupts. 
///    - Example for initializing SPI interrupt handler in upper application.
/// \code
///	      AIC_ConfigureIT(AT91C_ID_SPI, 0, SPI_Handler);
/// \endcode
//------------------------------------------------------------------------------

#ifndef SPID_H
#define SPID_H

//------------------------------------------------------------------------------
//         Headers
//------------------------------------------------------------------------------

#include <board.h>

//------------------------------------------------------------------------------
//         Definitions
//------------------------------------------------------------------------------

/// An unspecified error has occured.
#define SPID_ERROR          1

/// SPI driver is currently in use.
#define SPID_ERROR_LOCK     2

//------------------------------------------------------------------------------
//         Macros
//------------------------------------------------------------------------------

/// Calculates the value of the SCBR field of the Chip Select Register given
/// MCK and SPCK.
#define SPID_CSR_SCBR(mck, spck)    ((((mck) / (spck)) << 8) & AT91C_SPI_SCBR)

/// Calculates the value of the DLYBS field of the Chip Select Register given
/// the delay in ns and MCK.
#define SPID_CSR_DLYBS(mck, delay) \
    ((((((delay) * ((mck) / 1000000)) / 1000) + 1)  << 16) & AT91C_SPI_DLYBS)

/// Calculates the value of the DLYBCT field of the Chip Select Register given
/// the delay in ns and MCK.
#define SPID_CSR_DLYBCT(mck, delay) \
    ((((((delay) / 32 * ((mck) / 1000000)) / 1000) + 1) << 24) & AT91C_SPI_DLYBCT)

//------------------------------------------------------------------------------
//         Types
//------------------------------------------------------------------------------

/// SPI transfer complete callback.
typedef void (*SpidCallback )(unsigned char, void *);

//------------------------------------------------------------------------------
/// Spi Transfer Request prepared by the application upper layer. This structure
/// is sent to the SPI_SendCommand function to start the transfer. At the end of 
/// the transfer, the callback is invoked by the interrupt handler.
//------------------------------------------------------------------------------
typedef struct _SpidCmd {

    /// Pointer to the command data.
	unsigned char *pCmd;
    /// Command size in bytes.
	unsigned char cmdSize;
    /// Pointer to the data to be sent.
	unsigned char *pData;
    /// Data size in bytes.
	unsigned short dataSize;
    /// SPI chip select.
	unsigned char spiCs;
    /// Callback function invoked at the end of transfer.
	SpidCallback callback;
    /// Callback arguments.
	void *pArgument;

} SpidCmd;

//------------------------------------------------------------------------------
/// Constant structure associated with SPI port. This structure prevents
/// client applications to have access in the same time.
//------------------------------------------------------------------------------
typedef struct {

    /// Pointer to SPI Hardware registers
	AT91S_SPI *pSpiHw;
    /// SPI Id as defined in the product datasheet
	char spiId;
    /// Current SpiCommand being processed
	SpidCmd *pCurrentCommand;
    /// Mutual exclusion semaphore.
	volatile char semaphore;

} Spid;

//------------------------------------------------------------------------------
//         Exported functions
//------------------------------------------------------------------------------

extern unsigned char SPID_Configure(
    Spid *pSpid,
    AT91S_SPI *pSpiHw,
    unsigned char spiId);

extern void SPID_ConfigureCS(Spid *pSpid, unsigned char cs, unsigned int csr);
	
extern unsigned char SPID_SendCommand(
	Spid *pSpid,
	SpidCmd *pCommand);

extern void SPID_Handler(Spid *pSpid);

extern unsigned char SPID_IsBusy(const Spid *pSpid);

#endif // #ifndef SPID_H

