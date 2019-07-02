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
//      Headers
//------------------------------------------------------------------------------

#include "SBCMethods.h"
#include "MSDDStateMachine.h"
#include <usb/device/core/USBD.h>

//------------------------------------------------------------------------------
//      Global variables
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//! \brief  Header for the mode pages data
//! \see    SBCModeParameterHeader6
//------------------------------------------------------------------------------
static const SBCModeParameterHeader6 modeParameterHeader6 = {

    sizeof(SBCModeParameterHeader6) - 1,        //! Length is 0x03
    SBC_MEDIUM_TYPE_DIRECT_ACCESS_BLOCK_DEVICE, //! Direct-access block device
    0,                                          //! Reserved bits
    0,                                          //! DPO/FUA not supported
    0,                                          //! Reserved bits
    0,                                          //! not write-protected
    0                                           //! No block descriptor
};

//------------------------------------------------------------------------------
//      Internal functions
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//! \brief  Performs a WRITE (10) command on the specified LUN.
//!
//!         The data to write is first received from the USB host and then
//!         actually written on the media.
//!         This function operates asynchronously and must be called multiple
//!         times to complete. A result code of MSDDriver_STATUS_INCOMPLETE
//!         indicates that at least another call of the method is necessary.
//! \param  lun          Pointer to the LUN affected by the command
//! \param  commandState Current state of the command
//! \return Operation result code (SUCCESS, ERROR, INCOMPLETE or PARAMETER)
//! \see    MSDLun
//! \see    MSDCommandState
//------------------------------------------------------------------------------
static unsigned char SBC_Write10(MSDLun          *lun,
                                 MSDCommandState *commandState)
{
    unsigned char  status;
    unsigned char  result = MSDD_STATUS_INCOMPLETE;
    MSDTransfer *transfer = &(commandState->transfer);
    SBCWrite10 *command = (SBCWrite10 *) commandState->cbw.pCommand;

    // Init command state
    if (commandState->state == 0) {

        commandState->state = SBC_STATE_READ;
    }

#if !defined(AT91C_EBI_SDRAM) && !defined(BOARD_USB_UDPHS) 
    // Convert length from bytes to blocks
    commandState->length /= lun->blockSize;
#endif

    // Check if length equals 0
    if (commandState->length == 0) {

        TRACE_INFO_WP("End ");
        result = MSDD_STATUS_SUCCESS;
    }
    else {

        // Current command status
        switch (commandState->state) {
        //------------------
        case SBC_STATE_READ:
        //------------------
            TRACE_INFO_WP("Receive ");
#if !defined(AT91C_EBI_SDRAM) && !defined(BOARD_USB_UDPHS) 
            // Read one block of data sent by the host
            status = MSDD_Read((void*)lun->readWriteBuffer,
                               lun->blockSize,
                               (TransferCallback) MSDDriver_Callback,
                               (void *) transfer);
#else
            status = MSDD_Read((void*)(lun->media->baseAddress
                                   + lun->baseAddress
                                   + DWORDB(command->pLogicalBlockAddress) * lun->blockSize),
                                commandState->length,
                                (TransferCallback) MSDDriver_Callback,
                                (void *) transfer);
#endif

            // Check operation result code
            if (status != USBD_STATUS_SUCCESS) {

                TRACE_WARNING(
                    "RBC_Write10: Failed to start receiving data\n\r");
                SBC_UpdateSenseData(&(lun->requestSenseData),
                                    SBC_SENSE_KEY_HARDWARE_ERROR,
                                    0,
                                    0);
                result = MSDD_STATUS_ERROR;
            }
            else {

                // Prepare next device state
                commandState->state = SBC_STATE_WAIT_READ;
            }
            break;

        //-----------------------
        case SBC_STATE_WAIT_READ:
        //-----------------------
            TRACE_INFO_WP("Wait ");

            // Check semaphore
            if (transfer->semaphore > 0) {

                transfer->semaphore--;
                commandState->state = SBC_STATE_WRITE;
            }
            break;

        //-------------------
        case SBC_STATE_WRITE:
        //-------------------
            // Check the result code of the read operation
            if (transfer->status != USBD_STATUS_SUCCESS) {

                TRACE_WARNING(
                    "RBC_Write10: Failed to received data\n\r");
                SBC_UpdateSenseData(&(lun->requestSenseData),
                                    SBC_SENSE_KEY_HARDWARE_ERROR,
                                    0,
                                    0);
                result = MSDD_STATUS_ERROR;
            }
            else {

#if !defined(AT91C_EBI_SDRAM) && !defined(BOARD_USB_UDPHS) 
                // Write the block to the media
                status = LUN_Write(lun,
                                    DWORDB(command->pLogicalBlockAddress),
                                    lun->readWriteBuffer,
                                    1,
                                    (TransferCallback) MSDDriver_Callback,
                                    (void *) transfer);
#else
                MSDDriver_Callback(transfer, MED_STATUS_SUCCESS, 0, 0);
                status = LUN_STATUS_SUCCESS;
#endif

                // Check operation result code
                if (status != USBD_STATUS_SUCCESS) {

                    TRACE_WARNING(
                        "RBC_Write10: Failed to start media write\n\r");
                    SBC_UpdateSenseData(&(lun->requestSenseData),
                                        SBC_SENSE_KEY_NOT_READY,
                                        0,
                                        0);
                    result = MSDD_STATUS_ERROR;
                }
                else {

                    // Prepare next state
                    commandState->state = SBC_STATE_WAIT_WRITE;
                }
            }
            break;

        //------------------------
        case SBC_STATE_WAIT_WRITE:
        //------------------------
            TRACE_INFO_WP("Wait ");

            // Check semaphore value
            if (transfer->semaphore > 0) {

                // Take semaphore and move to next state
                transfer->semaphore--;
                commandState->state = SBC_STATE_NEXT_BLOCK;
            }
            break;

        //------------------------
        case SBC_STATE_NEXT_BLOCK:
        //------------------------
            // Check operation result code
            if (transfer->status != USBD_STATUS_SUCCESS) {

                TRACE_WARNING(
                    "RBC_Write10: Failed to write media\n\r");
                SBC_UpdateSenseData(&(lun->requestSenseData),
                                    SBC_SENSE_KEY_RECOVERED_ERROR,
                                    SBC_ASC_TOO_MUCH_WRITE_DATA,
                                    0);
                result = MSDD_STATUS_ERROR;
            }
            else {

                // Update transfer length and block address
#if !defined(AT91C_EBI_SDRAM) && !defined(BOARD_USB_UDPHS) 
                commandState->length--;
#else
                commandState->length = 0;
#endif
                STORE_DWORDB(DWORDB(command->pLogicalBlockAddress) + 1,
                             command->pLogicalBlockAddress);

                // Check if transfer is finished
                if (commandState->length == 0) {

                    result = MSDD_STATUS_SUCCESS;
                }
                else {

                    commandState->state = SBC_STATE_READ;
                }
            }
            break;
        }
    }

#if !defined(AT91C_EBI_SDRAM) && !defined(BOARD_USB_UDPHS) 
    // Convert length from blocks to bytes
    commandState->length *= lun->blockSize;
#endif

    return result;
}

//------------------------------------------------------------------------------
//! \brief  Performs a READ (10) command on specified LUN.
//!
//!         The data is first read from the media and then sent to the USB host.
//!         This function operates asynchronously and must be called multiple
//!         times to complete. A result code of MSDDriver_STATUS_INCOMPLETE
//!         indicates that at least another call of the method is necessary.
//! \param  lun          Pointer to the LUN affected by the command
//! \param  commandState Current state of the command
//! \return Operation result code (SUCCESS, ERROR, INCOMPLETE or PARAMETER)
//! \see    MSDLun
//! \see    MSDCommandState
//------------------------------------------------------------------------------
static unsigned char SBC_Read10(MSDLun          *lun,
                                MSDCommandState *commandState)
{
    unsigned char status;
    unsigned char result = MSDD_STATUS_INCOMPLETE;
    SBCRead10 *command = (SBCRead10 *) commandState->cbw.pCommand;
    MSDTransfer *transfer = &(commandState->transfer);

    // Init command state
    if (commandState->state == 0) {

        commandState->state = SBC_STATE_READ;
    }

#if !defined(AT91C_EBI_SDRAM) && !defined(BOARD_USB_UDPHS) 
    // Convert length from bytes to blocks
    commandState->length /= lun->blockSize;
#endif

    // Check length
    if (commandState->length == 0) {

        result = MSDD_STATUS_SUCCESS;
    }
    else {

        // Command state management
        switch (commandState->state) {
        //------------------
        case SBC_STATE_READ:
        //------------------
            // Read one block of data from the media
#if !defined(AT91C_EBI_SDRAM) && !defined(BOARD_USB_UDPHS) 
            status = LUN_Read(lun,
                               DWORDB(command->pLogicalBlockAddress),
                               lun->readWriteBuffer,
                               1,
                               (TransferCallback) MSDDriver_Callback,
                               (void *) transfer);
#else
            MSDDriver_Callback(transfer, MED_STATUS_SUCCESS, 0, 0);
            status = LUN_STATUS_SUCCESS;
#endif
            // Check operation result code
            if (status != LUN_STATUS_SUCCESS) {

                TRACE_WARNING(
                    "RBC_Read10: Failed to start reading media\n\r");
                SBC_UpdateSenseData(&(lun->requestSenseData),
                                    SBC_SENSE_KEY_NOT_READY,
                                    SBC_ASC_LOGICAL_UNIT_NOT_READY,
                                    0);
                result = MSDD_STATUS_ERROR;
            }
            else {

                // Move to next command state
                commandState->state = SBC_STATE_WAIT_READ;
            }
            break;

        //-----------------------
        case SBC_STATE_WAIT_READ:
        //-----------------------
            // Check semaphore value
            if (transfer->semaphore > 0) {

                TRACE_INFO_WP("Ok ");

                // Take semaphore and move to next state
                transfer->semaphore--;
                commandState->state = SBC_STATE_WRITE;
            }
            break;

        //-------------------
        case SBC_STATE_WRITE:
        //-------------------
            // Check the operation result code
            if (transfer->status != USBD_STATUS_SUCCESS) {

                TRACE_WARNING(
                    "RBC_Read10: Failed to read media\n\r");
                SBC_UpdateSenseData(&(lun->requestSenseData),
                                    SBC_SENSE_KEY_RECOVERED_ERROR,
                                    SBC_ASC_LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE,
                                    0);
                result = MSDD_STATUS_ERROR;
            }
            else {

                // Send the block to the host
#if !defined(AT91C_EBI_SDRAM) && !defined(BOARD_USB_UDPHS) 
                status = MSDD_Write((void*)lun->readWriteBuffer,
                                    lun->blockSize,
                                    (TransferCallback) MSDDriver_Callback,
                                    (void *) transfer);
#else

                status = MSDD_Write((void*)(lun->media->baseAddress
                                       + lun->baseAddress
                                       + DWORDB(command->pLogicalBlockAddress) * lun->blockSize),
                                    commandState->length,
                                    (TransferCallback) MSDDriver_Callback,
                                    (void *) transfer);

#endif

                // Check operation result code
                if (status != USBD_STATUS_SUCCESS) {

                    TRACE_WARNING(
                        "RBC_Read10: Failed to start to send data\n\r");
                    SBC_UpdateSenseData(&(lun->requestSenseData),
                                        SBC_SENSE_KEY_HARDWARE_ERROR,
                                        0,
                                        0);
                    result = MSDD_STATUS_ERROR;
                }
                else {

                    TRACE_INFO_WP("Sending ");

                    // Move to next command state
                    commandState->state = SBC_STATE_WAIT_WRITE;
                }
            }
            break;

        //------------------------
        case SBC_STATE_WAIT_WRITE:
        //------------------------
            // Check semaphore value
            if (transfer->semaphore > 0) {

                TRACE_INFO_WP("Sent ");

                // Take semaphore and move to next state
                transfer->semaphore--;
                commandState->state = SBC_STATE_NEXT_BLOCK;
            }
            break;

        //------------------------------
        case SBC_STATE_NEXT_BLOCK:
        //------------------------------
            // Check operation result code
            if (transfer->status != USBD_STATUS_SUCCESS) {

                TRACE_WARNING(
                    "RBC_Read10: Failed to send data\n\r");
                SBC_UpdateSenseData(&(lun->requestSenseData),
                                    SBC_SENSE_KEY_HARDWARE_ERROR,
                                    0,
                                    0);
                result = MSDD_STATUS_ERROR;
            }
            else {
                TRACE_INFO_WP("Next ");

                // Update transfer length and block address
                STORE_DWORDB(DWORDB(command->pLogicalBlockAddress) + 1,
                             command->pLogicalBlockAddress);
#if !defined(AT91C_EBI_SDRAM) && !defined(BOARD_USB_UDPHS) 
                commandState->length--;
#else
                commandState->length = 0;
#endif

                // Check if transfer is finished
                if (commandState->length == 0) {

                    result = MSDD_STATUS_SUCCESS;
                }
                else {

                    commandState->state = SBC_STATE_READ;
                }
            }
            break;
        }
    }

#if !defined(AT91C_EBI_SDRAM) && !defined(BOARD_USB_UDPHS) 
    // Convert length from blocks to bytes
    commandState->length *= lun->blockSize;
#endif

    return result;
}

//------------------------------------------------------------------------------
//! \brief  Performs a READ CAPACITY (10) command.
//!
//!         This function operates asynchronously and must be called multiple
//!         times to complete. A result code of MSDDriver_STATUS_INCOMPLETE
//!         indicates that at least another call of the method is necessary.
//! \param  lun          Pointer to the LUN affected by the command
//! \param  commandState Current state of the command
//! \return Operation result code (SUCCESS, ERROR, INCOMPLETE or PARAMETER)
//! \see    MSDLun
//! \see    MSDCommandState
//------------------------------------------------------------------------------
static unsigned char SBC_ReadCapacity10(MSDLun               *lun,
                                        MSDCommandState *commandState)
{
    unsigned char result = MSDD_STATUS_INCOMPLETE;
    unsigned char status;
    MSDTransfer *transfer = &(commandState->transfer);

    // Initialize command state if needed
    if (commandState->state == 0) {

        commandState->state = SBC_STATE_WRITE;
    }

    // Identify current command state
    switch (commandState->state) {
    //-------------------
    case SBC_STATE_WRITE:
    //-------------------
        // Start the write operation
        status = MSDD_Write(&(lun->readCapacityData),
                            commandState->length,
                            (TransferCallback) MSDDriver_Callback,
                            (void *) transfer);

        // Check operation result code
        if (status != USBD_STATUS_SUCCESS) {

            TRACE_WARNING(
                "RBC_ReadCapacity: Cannot start sending data\n\r");
            result = MSDD_STATUS_ERROR;
        }
        else {

            // Proceed to next command state
            TRACE_INFO_WP("Sending ");
            commandState->state = SBC_STATE_WAIT_WRITE;
        }
        break;

    //------------------------
    case SBC_STATE_WAIT_WRITE:
    //------------------------
        // Check semaphore value
        if (transfer->semaphore > 0) {

            // Take semaphore and terminate command
            transfer->semaphore--;

            if (transfer->status != USBD_STATUS_SUCCESS) {

                TRACE_WARNING("RBC_ReadCapacity: Cannot send data\n\r");
                result = MSDD_STATUS_ERROR;
            }
            else {

                TRACE_INFO_WP("Sent ");
                result = MSDD_STATUS_SUCCESS;
            }
            commandState->length -= transfer->transferred;
        }
        break;
    }

    return result;
}

//------------------------------------------------------------------------------
//! \brief  Handles an INQUIRY command.
//!
//!         This function operates asynchronously and must be called multiple
//!         times to complete. A result code of MSDDriver_STATUS_INCOMPLETE
//!         indicates that at least another call of the method is necessary.
//! \param  lun          Pointer to the LUN affected by the command
//! \param  commandState Current state of the command
//! \return Operation result code (SUCCESS, ERROR, INCOMPLETE or PARAMETER)
//! \see    MSDLun
//! \see    MSDCommandState
//------------------------------------------------------------------------------
static unsigned char SBC_Inquiry(MSDLun               *lun,
                                 MSDCommandState *commandState)
{
    unsigned char  result = MSDD_STATUS_INCOMPLETE;
    unsigned char  status;
    MSDTransfer *transfer = &(commandState->transfer);

    // Check if required length is 0
    if (commandState->length == 0) {

        // Nothing to do
        result = MSDD_STATUS_SUCCESS;
    }
    // Initialize command state if needed
    else if (commandState->state == 0) {

        commandState->state = SBC_STATE_WRITE;

        // Change additional length field of inquiry data
        lun->inquiryData->bAdditionalLength
            = (unsigned char) (commandState->length - 5);
    }

    // Identify current command state
    switch (commandState->state) {
    //-------------------
    case SBC_STATE_WRITE:
    //-------------------
        // Start write operation
        status = MSDD_Write((void *) lun->inquiryData,
                            commandState->length,
                            (TransferCallback) MSDDriver_Callback,
                            (void *) transfer);

        // Check operation result code
        if (status != USBD_STATUS_SUCCESS) {

            TRACE_WARNING(
                "SPC_Inquiry: Cannot start sending data\n\r");
            result = MSDD_STATUS_ERROR;
        }
        else {

            // Proceed to next state
            TRACE_INFO_WP("Sending ");
            commandState->state = SBC_STATE_WAIT_WRITE;
        }
        break;

    //------------------------
    case SBC_STATE_WAIT_WRITE:
    //------------------------
        // Check the semaphore value
        if (transfer->semaphore > 0) {

            // Take semaphore and terminate command
            transfer->semaphore--;

            if (transfer->status != USBD_STATUS_SUCCESS) {

                TRACE_WARNING(
                    "SPC_Inquiry: Data transfer failed\n\r");
                result = MSDD_STATUS_ERROR;
            }
            else {

                TRACE_INFO_WP("Sent ");
                result = MSDD_STATUS_SUCCESS;
            }

            // Update length field
            commandState->length -= transfer->transferred;
        }
        break;
    }

    return result;
}

//------------------------------------------------------------------------------
//! \brief  Performs a REQUEST SENSE command.
//!
//!         This function operates asynchronously and must be called multiple
//!         times to complete. A result code of MSDDriver_STATUS_INCOMPLETE
//!         indicates that at least another call of the method is necessary.
//! \param  lun          Pointer to the LUN affected by the command
//! \param  commandState Current state of the command
//! \return Operation result code (SUCCESS, ERROR, INCOMPLETE or PARAMETER)
//! \see    MSDLun
//! \see    MSDCommandState
//------------------------------------------------------------------------------
static unsigned char SBC_RequestSense(MSDLun               *lun,
                                      MSDCommandState *commandState)
{
    unsigned char result = MSDD_STATUS_INCOMPLETE;
    unsigned char status;
    MSDTransfer *transfer = &(commandState->transfer);

    // Check if requested length is zero
    if (commandState->length == 0) {

        // Nothing to do
        result = MSDD_STATUS_SUCCESS;
    }
    // Initialize command state if needed
    else if (commandState->state == 0) {

        commandState->state = SBC_STATE_WRITE;
    }

    // Identify current command state
    switch (commandState->state) {
    //-------------------
    case SBC_STATE_WRITE:
    //-------------------
        // Start transfer
        status = MSDD_Write(&(lun->requestSenseData),
                            commandState->length,
                            (TransferCallback) MSDDriver_Callback,
                            (void *) transfer);

        // Check result code
        if (status != USBD_STATUS_SUCCESS) {

            TRACE_WARNING(
                "RBC_RequestSense: Cannot start sending data\n\r");
            result = MSDD_STATUS_ERROR;
        }
        else {

            // Change state
            commandState->state = SBC_STATE_WAIT_WRITE;
        }
        break;

    //------------------------
    case SBC_STATE_WAIT_WRITE:
    //------------------------
        // Check the transfer semaphore
        if (transfer->semaphore > 0) {

            // Take semaphore and finish command
            transfer->semaphore--;

            if (transfer->status != USBD_STATUS_SUCCESS) {

                result = MSDD_STATUS_ERROR;
            }
            else {

                result = MSDD_STATUS_SUCCESS;
            }

            // Update length
            commandState->length -= transfer->transferred;
        }
        break;
    }

    return result;
}

//------------------------------------------------------------------------------
//! \brief  Performs a MODE SENSE (6) command.
//!
//!         This function operates asynchronously and must be called multiple
//!         times to complete. A result code of MSDDriver_STATUS_INCOMPLETE
//!         indicates that at least another call of the method is necessary.
//! \param  lun          Pointer to the LUN affected by the command
//! \param  commandState Current state of the command
//! \return Operation result code (SUCCESS, ERROR, INCOMPLETE or PARAMETER)
//! \see    MSDLun
//! \see    MSDCommandState
//------------------------------------------------------------------------------
static unsigned char SBC_ModeSense6(MSDCommandState *commandState)
{
    unsigned char      result = MSDD_STATUS_INCOMPLETE;
    unsigned char      status;
    MSDTransfer     *transfer = &(commandState->transfer);

    // Check if mode page is supported
    if (((SBCCommand *) commandState->cbw.pCommand)->modeSense6.bPageCode
        != SBC_PAGE_RETURN_ALL) {

        return MSDD_STATUS_PARAMETER;
    }

    // Initialize command state if needed
    if (commandState->state == 0) {

        commandState->state = SBC_STATE_WRITE;
    }

    // Check current command state
    switch (commandState->state) {
    //-------------------
    case SBC_STATE_WRITE:
    //-------------------
        // Start transfer
        status = MSDD_Write((void *) &modeParameterHeader6,
                            commandState->length,
                            (TransferCallback) MSDDriver_Callback,
                            (void *) transfer);

        // Check operation result code
        if (status != USBD_STATUS_SUCCESS) {

            TRACE_WARNING(
                "SPC_ModeSense6: Cannot start data transfer\n\r");
            result = MSDD_STATUS_ERROR;
        }
        else {

            // Proceed to next state
            commandState->state = SBC_STATE_WAIT_WRITE;
        }
        break;

    //------------------------
    case SBC_STATE_WAIT_WRITE:
    //------------------------
        TRACE_INFO_WP("Wait ");

        // Check semaphore value
        if (transfer->semaphore > 0) {

            // Take semaphore and terminate command
            transfer->semaphore--;

            if (transfer->status != USBD_STATUS_SUCCESS) {

                TRACE_WARNING(
                    "SPC_ModeSense6: Data transfer failed\n\r");
                result = MSDD_STATUS_ERROR;
            }
            else {

                result = MSDD_STATUS_SUCCESS;
            }

            // Update length field
            commandState->length -= transfer->transferred;

        }
        break;
    }

    return result;
}

//------------------------------------------------------------------------------
//! \brief  Performs a TEST UNIT READY COMMAND command.
//! \param  lun          Pointer to the LUN affected by the command
//! \return Operation result code (SUCCESS, ERROR, INCOMPLETE or PARAMETER)
//! \see    MSDLun
//------------------------------------------------------------------------------
static unsigned char SBC_TestUnitReady(MSDLun *lun)
{
    unsigned char result = MSDD_STATUS_ERROR;

    // Check current media state
    switch(lun->media->state) {
    //-------------------
    case MED_STATE_READY:
    //-------------------
        // Nothing to do
        //TRACE_INFO_WP("Rdy ");
        result = MSDD_STATUS_SUCCESS;
        break;

    //------------------
    case MED_STATE_BUSY:
    //------------------
        TRACE_INFO_WP("Bsy ");
        SBC_UpdateSenseData(&(lun->requestSenseData),
                            SBC_SENSE_KEY_NOT_READY,
                            0,
                            0);
        break;

    //------
    default:
    //------
        TRACE_INFO_WP("? ");
        SBC_UpdateSenseData(&(lun->requestSenseData),
                            SBC_SENSE_KEY_NOT_READY,
                            SBC_ASC_MEDIUM_NOT_PRESENT,
                            0);
        break;
    }

    return result;
}

//------------------------------------------------------------------------------
//      Exported functions
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//! \brief  Updates the sense data of a LUN with the given key and codes
//! \param  requestSenseData             Pointer to the sense data to update
//! \param  senseKey                     Sense key
//! \param  additionalSenseCode          Additional sense code
//! \param  additionalSenseCodeQualifier Additional sense code qualifier
//------------------------------------------------------------------------------
void SBC_UpdateSenseData(SBCRequestSenseData *requestSenseData,
                         unsigned char senseKey,
                         unsigned char additionalSenseCode,
                         unsigned char additionalSenseCodeQualifier)
{
    requestSenseData->bSenseKey = senseKey;
    requestSenseData->bAdditionalSenseCode = additionalSenseCode;
    requestSenseData->bAdditionalSenseCodeQualifier
        = additionalSenseCodeQualifier;
}

//------------------------------------------------------------------------------
//! \brief  Return information about the transfer length and direction expected
//!         by the device for a particular command.
//! \param  command Pointer to a buffer holding the command to evaluate
//! \param  length  Expected length of the data transfer
//! \param  type    Expected direction of data transfer
//! \param  lun     Pointer to the LUN affected by the command
//------------------------------------------------------------------------------
unsigned char SBC_GetCommandInformation(void          *command,
                               unsigned int  *length,
                               unsigned char *type,
                               MSDLun         *lun)
{
    SBCCommand *sbcCommand = (SBCCommand *) command;
    unsigned char          isCommandSupported = 1;

    // Identify command
    switch (sbcCommand->bOperationCode) {
    //---------------
    case SBC_INQUIRY:
    //---------------
        (*type) = MSDD_DEVICE_TO_HOST;

        // Allocation length is stored in big-endian format
        (*length) = WORDB(sbcCommand->inquiry.pAllocationLength);
        break;

    //--------------------
    case SBC_MODE_SENSE_6:
    //--------------------
        (*type) = MSDD_DEVICE_TO_HOST;
        if (sbcCommand->modeSense6.bAllocationLength >
            sizeof(SBCModeParameterHeader6)) {

            *length = sizeof(SBCModeParameterHeader6);
        }
        else {

            *length = sbcCommand->modeSense6.bAllocationLength;
        }

        // Only "return all pages" command is supported
        if (sbcCommand->modeSense6.bPageCode != SBC_PAGE_RETURN_ALL) {

            // Unsupported page
            TRACE_WARNING(
            "SBC_GetCommandInformation: Page code not supported(0x%02X)\n\r",
                          sbcCommand->modeSense6.bPageCode);
            isCommandSupported = 0;
            (*length) = 0;
        }
        break;

    //------------------------------------
    case SBC_PREVENT_ALLOW_MEDIUM_REMOVAL:
    //------------------------------------
        (*type) = MSDD_NO_TRANSFER;
        break;

    //---------------------
    case SBC_REQUEST_SENSE:
    //---------------------
        (*type) = MSDD_DEVICE_TO_HOST;
        (*length) = sbcCommand->requestSense.bAllocationLength;
        break;

    //-----------------------
    case SBC_TEST_UNIT_READY:
    //-----------------------
        (*type) = MSDD_NO_TRANSFER;
        break;

    //---------------------
    case SBC_READ_CAPACITY_10:
    //---------------------
        (*type) = MSDD_DEVICE_TO_HOST;
        (*length) = sizeof(SBCReadCapacity10Data);
        break;

    //---------------
    case SBC_READ_10:
    //---------------
        (*type) = MSDD_DEVICE_TO_HOST;
        (*length) = WORDB(sbcCommand->read10.pTransferLength)
                     * lun->blockSize;
        break;

    //----------------
    case SBC_WRITE_10:
    //----------------
        (*type) = MSDD_HOST_TO_DEVICE;
        (*length) = WORDB(sbcCommand->write10.pTransferLength)
                     * lun->blockSize;
        break;

    //-----------------
    case SBC_VERIFY_10:
    //-----------------
        (*type) = MSDD_NO_TRANSFER;
        break;

    //------
    default:
    //------
        isCommandSupported = 0;
    }

    // If length is 0, no transfer is expected
    if ((*length) == 0) {

        (*type) = MSDD_NO_TRANSFER;
    }

    return isCommandSupported;
}

//------------------------------------------------------------------------------
//! \brief  Processes a SBC command by dispatching it to a subfunction.
//! \param  lun          Pointer to the affected LUN
//! \param  commandState Pointer to the current command state
//! \return Operation result code
//------------------------------------------------------------------------------
unsigned char SBC_ProcessCommand(MSDLun               *lun,
                                 MSDCommandState *commandState)
{
    unsigned char result = MSDD_STATUS_INCOMPLETE;
    SBCCommand *command = (SBCCommand *) commandState->cbw.pCommand;

    // Identify command
    switch (command->bOperationCode) {
    //---------------
    case SBC_READ_10:
    //---------------
        TRACE_INFO_WP("Read(10) ");

        // Perform the Read10 command
        result = SBC_Read10(lun, commandState);
        break;

    //----------------
    case SBC_WRITE_10:
    //----------------
        TRACE_INFO_WP("Write(10) ");

        // Perform the Write10 command
        result = SBC_Write10(lun, commandState);
        break;

    //---------------------
    case SBC_READ_CAPACITY_10:
    //---------------------
        TRACE_INFO_WP("RdCapacity(10) ");

        // Perform the ReadCapacity command
        result = SBC_ReadCapacity10(lun, commandState);
        break;

    //---------------------
    case SBC_VERIFY_10:
    //---------------------
        TRACE_INFO_WP("Verify(10) ");

        // Flush media
        MED_Flush(lun->media);
        result = MSDD_STATUS_SUCCESS;
        break;

    //---------------
    case SBC_INQUIRY:
    //---------------
        TRACE_INFO_WP("Inquiry ");

        // Process Inquiry command
        result = SBC_Inquiry(lun, commandState);
        break;

    //--------------------
    case SBC_MODE_SENSE_6:
    //--------------------
        TRACE_INFO_WP("ModeSense(6) ");

        // Process ModeSense6 command
        result = SBC_ModeSense6(commandState);
        break;

    //-----------------------
    case SBC_TEST_UNIT_READY:
    //-----------------------
        //TRACE_INFO_WP("TstUnitRdy ");

        // Process TestUnitReady command
        //MED_Flush(lun->media);
        //result = SBC_TestUnitReady(lun);
        result =MSDD_STATUS_SUCCESS;
        break;

    //---------------------
    case SBC_REQUEST_SENSE:
    //---------------------
        TRACE_INFO_WP("ReqSense ");

        // Perform the RequestSense command
        result = SBC_RequestSense(lun, commandState);
        break;

    //------------------------------------
    case SBC_PREVENT_ALLOW_MEDIUM_REMOVAL:
    //------------------------------------
        TRACE_INFO_WP("PrevAllowRem ");

        // Nothing to do
        result = MSDD_STATUS_SUCCESS;
        break;

    //------
    default:
    //------
        result = MSDD_STATUS_PARAMETER;
    }

    return result;
}
