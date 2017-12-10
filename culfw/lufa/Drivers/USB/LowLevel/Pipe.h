/*
             LUFA Library
     Copyright (C) Dean Camera, 2009.
              
  dean [at] fourwalledcubicle [dot] com
      www.fourwalledcubicle.com
*/

/*
  Copyright 2009  Dean Camera (dean [at] fourwalledcubicle [dot] com)

  Permission to use, copy, modify, and distribute this software
  and its documentation for any purpose and without fee is hereby
  granted, provided that the above copyright notice appear in all
  copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting
  documentation, and that the name of the author not be used in
  advertising or publicity pertaining to distribution of the
  software without specific, written prior permission.

  The author disclaim all warranties with regard to this
  software, including all implied warranties of merchantability
  and fitness.  In no event shall the author be liable for any
  special, indirect or consequential damages or any damages
  whatsoever resulting from loss of use, data or profits, whether
  in an action of contract, negligence or other tortious action,
  arising out of or in connection with the use or performance of
  this software.
*/
 
/** \ingroup Group_USB
 *  @defgroup Group_PipeManagement Pipe Management
 *
 *  This module contains functions, macros and enums related to pipe management when in USB Host mode. This
 *  module contains the pipe management macros, as well as pipe interrupt and data send/recieve functions
 *  for various data types.
 *
 *  @{
 */

/** @defgroup Group_PipeRW Pipe Data Reading and Writing
 *
 *  Functions, macros, variables, enums and types related to data reading and writing from and to pipes.
 */
 
/** \ingroup Group_PipeRW  
 *  @defgroup Group_PipePrimitiveRW Read/Write of Primitive Data Types
 *
 *  Functions, macros, variables, enums and types related to data reading and writing of primitive data types
 *  from and to pipes.
 */ 

/** \ingroup Group_PipeRW  
 *  @defgroup Group_PipeStreamRW Read/Write of Multi-Byte Streams
 *
 *  Functions, macros, variables, enums and types related to data reading and writing of data streams from
 *  and to pipes.
 */ 
 
/** @defgroup Group_PipePacketManagement Pipe Packet Management
 *
 *  Functions, macros, variables, enums and types related to packet management of pipes.
 */
 
/** @defgroup Group_PipeControlReq Pipe Control Request Management
 *
 *  Module for host mode request processing. This module allows for the transmission of standard, class and
 *  vendor control requests to the default control endpoint of an attached device while in host mode.
 *
 *  \see Chapter 9 of the USB 2.0 specification.
 */ 

#ifndef __PIPE_H__
#define __PIPE_H__

	/* Includes: */
		#include <avr/io.h>
		#include <avr/pgmspace.h>
		#include <avr/eeprom.h>
		#include <stdbool.h>

		#include "../../../Common/Common.h"
		#include "../HighLevel/USBTask.h"

		#if !defined(NO_STREAM_CALLBACKS) || defined(__DOXYGEN__)
			#include "../HighLevel/StreamCallbacks.h"
		#endif
		
	/* Enable C linkage for C++ Compilers: */
		#if defined(__cplusplus)
			extern "C" {
		#endif

	/* Public Interface - May be used in end-application: */
		/* Macros: */
			/** Mask for \ref Pipe_GetErrorFlags(), indicating that an overflow error occurred in the pipe on the received data. */
			#define PIPE_ERRORFLAG_OVERFLOW         (1 << 6)

			/** Mask for \ref Pipe_GetErrorFlags(), indicating that an underflow error occurred in the pipe on the received data. */
			#define PIPE_ERRORFLAG_UNDERFLOW        (1 << 5)

			/** Mask for \ref Pipe_GetErrorFlags(), indicating that a CRC error occurred in the pipe on the received data. */
			#define PIPE_ERRORFLAG_CRC16            (1 << 4)

			/** Mask for \ref Pipe_GetErrorFlags(), indicating that a hardware timeout error occurred in the pipe. */
			#define PIPE_ERRORFLAG_TIMEOUT          (1 << 3)

			/** Mask for \ref Pipe_GetErrorFlags(), indicating that a hardware PID error occurred in the pipe. */
			#define PIPE_ERRORFLAG_PID              (1 << 2)

			/** Mask for \ref Pipe_GetErrorFlags(), indicating that a hardware data PID error occurred in the pipe. */
			#define PIPE_ERRORFLAG_DATAPID          (1 << 1)

			/** Mask for \ref Pipe_GetErrorFlags(), indicating that a hardware data toggle error occurred in the pipe. */
			#define PIPE_ERRORFLAG_DATATGL          (1 << 0)

			/** Token mask for \ref Pipe_ConfigurePipe(). This sets the pipe as a SETUP token (for CONTROL type pipes),
			 *  which will trigger a control request on the attached device when data is written to the pipe.
			 */
			#define PIPE_TOKEN_SETUP                (0 << PTOKEN0)

			/** Token mask for \ref Pipe_ConfigurePipe(). This sets the pipe as a IN token (for non-CONTROL type pipes),
			 *  indicating that the pipe data will flow from device to host.
			 */
			#define PIPE_TOKEN_IN                   (1 << PTOKEN0)

			/** Token mask for \ref Pipe_ConfigurePipe(). This sets the pipe as a IN token (for non-CONTROL type pipes),
			 *  indicating that the pipe data will flow from host to device.
			 */
			#define PIPE_TOKEN_OUT                  (2 << PTOKEN0)

			/** Mask for the bank mode selection for the \ref Pipe_ConfigurePipe() macro. This indicates that the pipe
			 *  should have one single bank, which requires less USB FIFO memory but results in slower transfers as
			 *  only one USB device (the AVR or the attached device) can access the pipe's bank at the one time.
			 */
			#define PIPE_BANK_SINGLE                (0 << EPBK0)

			/** Mask for the bank mode selection for the \ref Pipe_ConfigurePipe() macro. This indicates that the pipe
			 *  should have two banks, which requires more USB FIFO memory but results in faster transfers as one
			 *  USB device (the AVR or the attached device) can access one bank while the other accesses the second
			 *  bank.
			 */
			#define PIPE_BANK_DOUBLE                (1 << EPBK0)
			
			/** Pipe address for the default control pipe, which always resides in address 0. This is
			 *  defined for convenience to give more readable code when used with the pipe macros.
			 */
			#define PIPE_CONTROLPIPE                0

			/** Default size of the default control pipe's bank, until altered by the Endpoint0Size value 
			 *  in the device descriptor of the attached device.
			 */
			#define PIPE_CONTROLPIPE_DEFAULT_SIZE   64
			
			/** Pipe number mask, for masking against pipe addresses to retrieve the pipe's numerical address
			 *  in the device.
			 */
			#define PIPE_PIPENUM_MASK               0x07

			/** Total number of pipes (including the default control pipe at address 0) which may be used in
			 *  the device. Different USB AVR models support different amounts of pipes, this value reflects
			 *  the maximum number of pipes for the currently selected AVR model.
			 */
			#define PIPE_TOTAL_PIPES                7

			/** Size in bytes of the largest pipe bank size possible in the device. Not all banks on each AVR
			 *  model supports the largest bank size possible on the device; different pipe numbers support
			 *  different maximum bank sizes. This value reflects the largest possible bank of any pipe on the
			 *  currently selected USB AVR model.
			 */
			#define PIPE_MAX_SIZE                   256

			/** Endpoint number mask, for masking against endpoint addresses to retrieve the endpoint's
			 *  numerical address in the attached device.
			 */
			#define PIPE_EPNUM_MASK                 0x0F

		/* Pseudo-Function Macros: */
			#if defined(__DOXYGEN__)
				/** Indicates the number of bytes currently stored in the current pipes's selected bank.
				 *
				 *  \note The return width of this function may differ, depending on the maximum pipe bank size
				 *        of the selected AVR model.
				 *
				 *  \ingroup Group_PipeRW
				 *
				 *  \return Total number of bytes in the currently selected Pipe's FIFO buffer
				 */
				static inline uint16_t Pipe_BytesInPipe(void);
				
				/** Returns the pipe address of the currently selected pipe. This is typically used to save the
				 *  currently selected pipe number so that it can be restored after another pipe has been manipulated.
				 *
				 *  \return Index of the currently selected pipe
				 */
				static inline uint8_t Pipe_GetCurrentPipe(void);

				/** Selects the given pipe number. Any pipe operations which do not require the pipe number to be
				 *  indicated will operate on the currently selected pipe.
				 *
				 *  \param[in] PipeNumber  Index of the pipe to select
				 */
				static inline void Pipe_SelectPipe(uint8_t PipeNumber);
				
				/** Resets the desired pipe, including the pipe banks and flags.
				 *
				 *  \param[in] PipeNumber  Index of the pipe to reset
				 */
				static inline void Pipe_ResetPipe(uint8_t PipeNumber);
				
				/** Enables the currently selected pipe so that data can be sent and received through it to and from
				 *  an attached device.
				 *
				 *  \note Pipes must first be configured properly via \ref Pipe_ConfigurePipe().
				 */
				static inline void Pipe_EnablePipe(void);

				/** Disables the currently selected pipe so that data cannot be sent and received through it to and
				 *  from an attached device.
				 */
				static inline void Pipe_DisablePipe(void);

				/** Determines if the currently selected pipe is enabled, but not necessarily configured.
				 *
				 * \return Boolean True if the currently selected pipe is enabled, false otherwise
				 */
				static inline bool Pipe_IsEnabled(void);
				
				/** Gets the current pipe token, indicating the pipe's data direction and type.
				 *
				 *  \return The current pipe token, as a PIPE_TOKEN_* mask
				 */
				static inline uint8_t Pipe_GetPipeToken(void);
				
				/** Sets the token for the currently selected pipe to one of the tokens specified by the PIPE_TOKEN_*
				 *  masks. This can be used on CONTROL type pipes, to allow for bidirectional transfer of data during
				 *  control requests, or on regular pipes to allow for half-duplex bidirectional data transfer to devices
				 *  which have two endpoints of opposite direction sharing the same endpoint address within the device.
				 *
				 *  \param[in] Token  New pipe token to set the selected pipe to, as a PIPE_TOKEN_* mask
				 */
				static inline void Pipe_SetPipeToken(uint8_t Token);
				
				/** Configures the currently selected pipe to allow for an unlimited number of IN requests. */
				static inline void Pipe_SetInfiniteINRequests(void);
				
				/** Configures the currently selected pipe to only allow the specified number of IN requests to be
				 *  accepted by the pipe before it is automatically frozen.
				 *
				 *  \param[in] TotalINRequests  Total number of IN requests that the pipe may receive before freezing
				 */
				static inline void Pipe_SetFiniteINRequests(uint8_t TotalINRequests);

				/** Determines if the currently selected pipe is configured.
				 *
				 *  \return Boolean true if the selected pipe is configured, false otherwise
				 */
				static inline bool Pipe_IsConfigured(void);
				
				/** Retrieves the endpoint number of the endpoint within the attached device that the currently selected
				 *  pipe is bound to.
				 *
				 *  \return Endpoint number the currently selected pipe is bound to
				 */
				static inline uint8_t Pipe_BoundEndpointNumber(void);

				/** Sets the period between interrupts for an INTERRUPT type pipe to a specified number of milliseconds.
				 *
				 *  \param[in] Milliseconds  Number of milliseconds between each pipe poll
				 */
				static inline void Pipe_SetInterruptPeriod(uint8_t Milliseconds);
				
				/** Returns a mask indicating which pipe's interrupt periods have elapsed, indicating that the pipe should
				 *  be serviced.
				 *
				 *  \return Mask whose bits indicate which pipes have interrupted
				 */
				static inline uint8_t Pipe_GetPipeInterrupts(void);
				
				/** Determines if the specified pipe number has interrupted (valid only for INTERRUPT type
				 *  pipes).
				 *
				 *  \param[in] PipeNumber  Index of the pipe whose interrupt flag should be tested
				 *
				 *  \return Boolean true if the specified pipe has interrupted, false otherwise
				 */
				static inline bool Pipe_HasPipeInterrupted(uint8_t PipeNumber);
				
				/** Unfreezes the selected pipe, allowing it to communicate with an attached device. */
				static inline void Pipe_Unfreeze(void);
				
				/** Freezes the selected pipe, preventing it from communicating with an attached device. */
				static inline void Pipe_Freeze(void);

				/** Clears the master pipe error flag. */
				static inline void Pipe_ClearError(void);
				
				/** Determines if the master pipe error flag is set for the currently selected pipe, indicating that
				 *  some sort of hardware error has occurred on the pipe.
				 *
				 *  \see \ref Pipe_GetErrorFlags() macro for information on retrieving the exact error flag.
				 *
				 *  \return Boolean true if an error has occurred on the selected pipe, false otherwise
				 */
				static inline bool Pipe_IsError(void);
				
				/** Clears all the currently selected pipe's hardware error flags, but does not clear the master error
				 *  flag for the pipe.
				 */
				static inline void Pipe_ClearErrorFlags(void);
				
				/** Gets a mask of the hardware error flags which have occurred on the currently selected pipe. This
				 *  value can then be masked against the PIPE_ERRORFLAG_* masks to determine what error has occurred.
				 *
				 *  \return  Mask comprising of PIPE_ERRORFLAG_* bits indicating what error has occurred on the selected pipe
				 */
				static inline uint8_t Pipe_GetErrorFlags(void);
				
				/** Determines if the currently selected pipe may be read from (if data is waiting in the pipe
				 *  bank and the pipe is an IN direction, or if the bank is not yet full if the pipe is an OUT
				 *  direction). This function will return false if an error has occurred in the pipe, or if the pipe
				 *  is an IN direction and no packet (or an empty packet) has been received, or if the pipe is an OUT
				 *  direction and the pipe bank is full.
				 *
				 *  \note This function is not valid on CONTROL type pipes.
				 *
				 *  \ingroup Group_PipePacketManagement
				 *  
				 *  \return Boolean true if the currently selected pipe may be read from or written to, depending on its direction
				 */
				static inline bool Pipe_IsReadWriteAllowed(void);
				
				/** Determines if an IN request has been received on the currently selected pipe.
				 *
				 *  \ingroup Group_PipePacketManagement
				 *
				 *  \return Boolean true if the current pipe has received an IN packet, false otherwise.
				 */
				static inline bool Pipe_IsINReceived(void);
				
				/** Determines if the currently selected pipe is ready to send an OUT request.
				 *
				 *  \ingroup Group_PipePacketManagement
				 *
				 *  \return Boolean true if the current pipe is ready for an OUT packet, false otherwise.
				 */
				static inline bool Pipe_IsOUTReady(void);

				/** Determines if no SETUP request is currently being sent to the attached device on the selected
				 *  CONTROL type pipe.
				 *
				 *  \ingroup Group_PipePacketManagement
				 *
				 *  \return Boolean true if the current pipe is ready for a SETUP packet, false otherwise.
				 */
				static inline bool Pipe_IsSETUPSent(void);
				
				/** Sends the currently selected CONTROL type pipe's contents to the device as a SETUP packet.
				 *
				 *  \ingroup Group_PipePacketManagement		
				 */
				static inline void Pipe_ClearSETUP(void);

				/** Acknowledges the reception of a setup IN request from the attached device on the currently selected
				 *  pipe, freeing the bank ready for the next packet.
				 *
				 *  \ingroup Group_PipePacketManagement
				 */
				static inline void Pipe_ClearIN(void);

				/** Sends the currently selected pipe's contents to the device as an OUT packet on the selected pipe, freeing
				 *  the bank ready for the next packet.
				 *
				 *  \ingroup Group_PipePacketManagement
				 */
				static inline void Pipe_ClearOUT(void);

				/** Determines if the device sent a NAK (Negative Acknowledge) in response to the last sent packet on
				 *  the currently selected pipe. This occurs when the host sends a packet to the device, but the device
				 *  is not currently ready to handle the packet (i.e. its endpoint banks are full). Once a NAK has been
				 *  received, it must be cleared using \ref Pipe_ClearNAKReceived() before the previous (or any other) packet
				 *  can be re-sent.
				 *
				 *  \ingroup Group_PipePacketManagement
				 *
				 *  \return Boolean true if an NAK has been received on the current pipe, false otherwise
				 */
				static inline bool Pipe_IsNAKReceived(void);

				/** Clears the NAK condition on the currently selected pipe.
				 *
				 *  \ingroup Group_PipePacketManagement
				 *
				 *  \see \ref Pipe_IsNAKReceived() for more details.
				 */
				static inline void Pipe_ClearNAKReceived(void);
				 
				/** Determines if the currently selected pipe has had the STALL condition set by the attached device.
				 *
				 *  \ingroup Group_PipePacketManagement
				 *
				 *  \return Boolean true if the current pipe has been stalled by the attached device, false otherwise
				*/
				static inline bool Pipe_IsStalled(void);
				
				/** Clears the STALL condition detection flag on the currently selected pipe, but does not clear the
				 *  STALL condition itself (this must be done via a ClearFeature control request to the device).
				 *
				 *  \ingroup Group_PipePacketManagement
				 */
				static inline void Pipe_ClearStall(void);
			#else
				#define Pipe_BytesInPipe()             UPBCX

				#define Pipe_GetCurrentPipe()          (UPNUM & PIPE_PIPENUM_MASK)

				#define Pipe_SelectPipe(pipenum)       MACROS{ UPNUM = pipenum; }MACROE
				
				#define Pipe_ResetPipe(pipenum)        MACROS{ UPRST = (1 << pipenum); UPRST = 0; }MACROE

				#define Pipe_EnablePipe()              MACROS{ UPCONX |= (1 << PEN); }MACROE

				#define Pipe_DisablePipe()             MACROS{ UPCONX &= ~(1 << PEN); }MACROE

				#define Pipe_IsEnabled()               ((UPCONX & (1 << PEN)) ? true : false)

				#define Pipe_GetPipeToken()            (UPCFG0X & PIPE_TOKEN_MASK)

				#define Pipe_SetToken(token)           MACROS{ UPCFG0X = ((UPCFG0X & ~PIPE_TOKEN_MASK) | token); }MACROE
				
				#define Pipe_SetInfiniteINRequests()   MACROS{ UPCONX |= (1 << INMODE); }MACROE

				#define Pipe_SetFiniteINRequests(n)    MACROS{ UPCONX &= ~(1 << INMODE); UPINRQX = n; }MACROE

				#define Pipe_IsConfigured()            ((UPSTAX  & (1 << CFGOK)) ? true : false)

				#define Pipe_BoundEndpointNumber()     ((UPCFG0X >> PEPNUM0) & PIPE_EPNUM_MASK)
				
				#define Pipe_SetInterruptPeriod(ms)    MACROS{ UPCFG2X = ms; }MACROE

				#define Pipe_GetPipeInterrupts()       UPINT

				#define Pipe_HasPipeInterrupted(n)     ((UPINT & (1 << n)) ? true : false)

				#define Pipe_Unfreeze()                MACROS{ UPCONX &= ~(1 << PFREEZE); }MACROE

				#define Pipe_Freeze()                  MACROS{ UPCONX |= (1 << PFREEZE); }MACROE

				#define Pipe_ClearError()              MACROS{ UPINTX &= ~(1 << PERRI); }MACROE

				#define Pipe_IsError()                 ((UPINTX & (1 << PERRI)) ? true : false)
				
				#define Pipe_ClearErrorFlags()         MACROS{ UPERRX = 0; }MACROE

				#define Pipe_GetErrorFlags()           ((UPERRX & (PIPE_ERRORFLAG_CRC16 | PIPE_ERRORFLAG_TIMEOUT | \
				                                                   PIPE_ERRORFLAG_PID   | PIPE_ERRORFLAG_DATAPID | \
				                                                   PIPE_ERRORFLAG_DATATGL))                      | \
				                                        (UPSTAX & PIPE_ERRORFLAG_OVERFLOW | PIPE_ERRORFLAG_UNDERFLOW))

				#define Pipe_IsReadWriteAllowed()      ((UPINTX & (1 << RWAL)) ? true : false)

				#define Pipe_IsINReceived()            ((UPINTX & (1 << RXINI)) ? true : false)

				#define Pipe_IsOUTReady()              ((UPINTX & (1 << TXOUTI)) ? true : false)

				#define Pipe_IsSETUPSent()             ((UPINTX & (1 << TXSTPI)) ? true : false)

				#define Pipe_ClearIN()                 MACROS{ uint8_t Temp = UPINTX; UPINTX = (Temp & ~(1 << RXINI)); \
				                                               UPINTX = (Temp & ~(1 << FIFOCON)); }MACROE

				#define Pipe_ClearOUT()                MACROS{ uint8_t Temp = UPINTX; UPINTX = (Temp & ~(1 << TXOUTI)); \
				                                               UPINTX = (Temp & ~(1 << FIFOCON)); }MACROE
				
				#define Pipe_ClearSETUP()              MACROS{ uint8_t Temp = UPINTX; UPINTX = (Temp & ~(1 << TXSTPI)); \
				                                               UPINTX = (Temp & ~(1 << FIFOCON)); }MACROE

				#define Pipe_IsNAKReceived()           ((UPINTX & (1 << NAKEDI)) ? true : false)

				#define Pipe_ClearNAKReceived()        MACROS{ UPINTX &= ~(1 << NAKEDI); }MACROE

				#define Pipe_IsStalled()               ((UPINTX & (1 << RXSTALLI)) ? true : false)

				#define Pipe_ClearStall()              MACROS{ UPINTX &= ~(1 << RXSTALLI); }MACROE
			#endif

		/* Enums: */
			/** Enum for the possible error return codes of the Pipe_WaitUntilReady function
			 *
			 *  \ingroup Group_PipeRW
			 */
			enum Pipe_WaitUntilReady_ErrorCodes_t
			{
				PIPE_READYWAIT_NoError                 = 0, /**< Pipe ready for next packet, no error */
				PIPE_READYWAIT_PipeStalled             = 1,	/**< The device stalled the pipe while waiting. */			
				PIPE_READYWAIT_DeviceDisconnected      = 2,	/**< Device was disconnected from the host while waiting. */
				PIPE_READYWAIT_Timeout                 = 3, /**< The device failed to accept or send the next packet
				                                             *   within the software timeout period set by the
				                                             *   \ref USB_STREAM_TIMEOUT_MS macro.
				                                             */
			};

			/** Enum for the possible error return codes of the Pipe_*_Stream_* functions.
			 *
			 *  \ingroup Group_PipeRW
			 */
			enum Pipe_Stream_RW_ErrorCodes_t
			{
				PIPE_RWSTREAM_NoError            = 0, /**< Command completed successfully, no error. */
				PIPE_RWSTREAM_PipeStalled        = 1, /**< The device stalled the pipe during the transfer. */		
				PIPE_RWSTREAM_DeviceDisconnected = 2, /**< Device was disconnected from the host during
			                                           *   the transfer.
			                                           */		
				PIPE_RWSTREAM_Timeout            = 3, /**< The device failed to accept or send the next packet
				                                       *   within the software timeout period set by the
				                                       *   \ref USB_STREAM_TIMEOUT_MS macro.
				                                       */
				PIPE_RWSTREAM_CallbackAborted    = 4, /**< Indicates that the stream's callback function aborted
			                                           *   the transfer early.
				                                       */
			};

		/* Inline Functions: */
			/** Reads one byte from the currently selected pipe's bank, for OUT direction pipes.
			 *
			 *  \ingroup Group_PipePrimitiveRW
			 *
			 *  \return Next byte in the currently selected pipe's FIFO buffer
			 */
			static inline uint8_t Pipe_Read_Byte(void) ATTR_WARN_UNUSED_RESULT ATTR_ALWAYS_INLINE;
			static inline uint8_t Pipe_Read_Byte(void)
			{
				return UPDATX;
			}

			/** Writes one byte from the currently selected pipe's bank, for IN direction pipes.
			 *
			 *  \ingroup Group_PipePrimitiveRW
			 *
			 *  \param[in] Byte  Next byte to write into the the currently selected pipe's FIFO buffer
			 */
			static inline void Pipe_Write_Byte(const uint8_t Byte) ATTR_ALWAYS_INLINE;
			static inline void Pipe_Write_Byte(const uint8_t Byte)
			{
				UPDATX = Byte;
			}

			/** Discards one byte from the currently selected pipe's bank, for OUT direction pipes.
			 *
			 *  \ingroup Group_PipePrimitiveRW
			 */
			static inline void Pipe_Discard_Byte(void) ATTR_ALWAYS_INLINE;
			static inline void Pipe_Discard_Byte(void)
			{
				uint8_t Dummy;
				
				Dummy = UPDATX;
			}
			
			/** Reads two bytes from the currently selected pipe's bank in little endian format, for OUT
			 *  direction pipes.
			 *
			 *  \ingroup Group_PipePrimitiveRW
			 *
			 *  \return Next word in the currently selected pipe's FIFO buffer
			 */
			static inline uint16_t Pipe_Read_Word_LE(void) ATTR_WARN_UNUSED_RESULT ATTR_ALWAYS_INLINE;
			static inline uint16_t Pipe_Read_Word_LE(void)
			{
				uint16_t Data;
				
				Data  = UPDATX;
				Data |= (((uint16_t)UPDATX) << 8);
			
				return Data;
			}

			/** Reads two bytes from the currently selected pipe's bank in big endian format, for OUT
			 *  direction pipes.
			 *
			 *  \ingroup Group_PipePrimitiveRW
			 *
			 *  \return Next word in the currently selected pipe's FIFO buffer
			 */
			static inline uint16_t Pipe_Read_Word_BE(void) ATTR_WARN_UNUSED_RESULT ATTR_ALWAYS_INLINE;
			static inline uint16_t Pipe_Read_Word_BE(void)
			{
				uint16_t Data;
				
				Data  = (((uint16_t)UPDATX) << 8);
				Data |= UPDATX;
			
				return Data;
			}
			
			/** Writes two bytes to the currently selected pipe's bank in little endian format, for IN
			 *  direction pipes.
			 *
			 *  \ingroup Group_PipePrimitiveRW
			 *
			 *  \param[in] Word  Next word to write to the currently selected pipe's FIFO buffer
			 */
			static inline void Pipe_Write_Word_LE(const uint16_t Word) ATTR_ALWAYS_INLINE;
			static inline void Pipe_Write_Word_LE(const uint16_t Word)
			{
				UPDATX = (Word & 0xFF);
				UPDATX = (Word >> 8);
			}
			
			/** Writes two bytes to the currently selected pipe's bank in big endian format, for IN
			 *  direction pipes.
			 *
			 *  \ingroup Group_PipePrimitiveRW
			 *
			 *  \param[in] Word  Next word to write to the currently selected pipe's FIFO buffer
			 */
			static inline void Pipe_Write_Word_BE(const uint16_t Word) ATTR_ALWAYS_INLINE;
			static inline void Pipe_Write_Word_BE(const uint16_t Word)
			{
				UPDATX = (Word >> 8);
				UPDATX = (Word & 0xFF);
			}

			/** Discards two bytes from the currently selected pipe's bank, for OUT direction pipes.
			 *
			 *  \ingroup Group_PipePrimitiveRW
			 */
			static inline void Pipe_Discard_Word(void) ATTR_ALWAYS_INLINE;
			static inline void Pipe_Discard_Word(void)
			{
				uint8_t Dummy;
				
				Dummy = UPDATX;
				Dummy = UPDATX;
			}

			/** Reads four bytes from the currently selected pipe's bank in little endian format, for OUT
			 *  direction pipes.
			 *
			 *  \ingroup Group_PipePrimitiveRW
			 *
			 *  \return Next double word in the currently selected pipe's FIFO buffer
			 */
			static inline uint32_t Pipe_Read_DWord_LE(void) ATTR_WARN_UNUSED_RESULT ATTR_ALWAYS_INLINE;
			static inline uint32_t Pipe_Read_DWord_LE(void)
			{
				union
				{
					uint32_t DWord;
					uint8_t  Bytes[4];
				} Data;
				
				Data.Bytes[0] = UPDATX;
				Data.Bytes[1] = UPDATX;
				Data.Bytes[2] = UPDATX;
				Data.Bytes[3] = UPDATX;
			
				return Data.DWord;
			}

			/** Reads four bytes from the currently selected pipe's bank in big endian format, for OUT
			 *  direction pipes.
			 *
			 *  \ingroup Group_PipePrimitiveRW
			 *
			 *  \return Next double word in the currently selected pipe's FIFO buffer
			 */
			static inline uint32_t Pipe_Read_DWord_BE(void) ATTR_WARN_UNUSED_RESULT ATTR_ALWAYS_INLINE;
			static inline uint32_t Pipe_Read_DWord_BE(void)
			{
				union
				{
					uint32_t DWord;
					uint8_t  Bytes[4];
				} Data;
				
				Data.Bytes[3] = UPDATX;
				Data.Bytes[2] = UPDATX;
				Data.Bytes[1] = UPDATX;
				Data.Bytes[0] = UPDATX;
			
				return Data.DWord;
			}

			/** Writes four bytes to the currently selected pipe's bank in little endian format, for IN
			 *  direction pipes.
			 *
			 *  \ingroup Group_PipePrimitiveRW
			 *
			 *  \param[in] DWord  Next double word to write to the currently selected pipe's FIFO buffer
			 */
			static inline void Pipe_Write_DWord_LE(const uint32_t DWord) ATTR_ALWAYS_INLINE;
			static inline void Pipe_Write_DWord_LE(const uint32_t DWord)
			{
				Pipe_Write_Word_LE(DWord);
				Pipe_Write_Word_LE(DWord >> 16);
			}
			
			/** Writes four bytes to the currently selected pipe's bank in big endian format, for IN
			 *  direction pipes.
			 *
			 *  \ingroup Group_PipePrimitiveRW
			 *
			 *  \param[in] DWord  Next double word to write to the currently selected pipe's FIFO buffer
			 */
			static inline void Pipe_Write_DWord_BE(const uint32_t DWord) ATTR_ALWAYS_INLINE;
			static inline void Pipe_Write_DWord_BE(const uint32_t DWord)
			{
				Pipe_Write_Word_BE(DWord >> 16);
				Pipe_Write_Word_BE(DWord);
			}			
			
			/** Discards four bytes from the currently selected pipe's bank, for OUT direction pipes.	
			 *
			 *  \ingroup Group_PipePrimitiveRW
			 */
			static inline void Pipe_Discard_DWord(void) ATTR_ALWAYS_INLINE;
			static inline void Pipe_Discard_DWord(void)
			{
				uint8_t Dummy;
				
				Dummy = UPDATX;
				Dummy = UPDATX;
				Dummy = UPDATX;
				Dummy = UPDATX;
			}

		/* External Variables: */
			/** Global indicating the maximum packet size of the default control pipe located at address
			 *  0 in the device. This value is set to the value indicated in the attached device's device
		     *  descriptor once the USB interface is initialized into host mode and a device is attached
			 *  to the USB bus.
			 *
			 *  \note This variable should be treated as read-only in the user application, and never manually
			 *        changed in value.
			 */
			extern uint8_t USB_ControlPipeSize;

		/* Function Prototypes: */
			#if !defined(NO_STREAM_CALLBACKS) || defined(__DOXYGEN__)
				#define _CALLBACK_PARAM     , StreamCallbackPtr_t Callback
			#else
				#define _CALLBACK_PARAM			
			#endif

			/** Configures the specified pipe number with the given pipe type, token, target endpoint number in the
			 *  attached device, bank size and banking mode. Pipes should be allocated in ascending order by their
			 *  address in the device (i.e. pipe 1 should be configured before pipe 2 and so on) to prevent fragmentation
			 *  of the USB FIFO memory.
			 *
			 *  The pipe type may be one of the EP_TYPE_* macros listed in LowLevel.h, the token may be one of the
			 *  PIPE_TOKEN_* masks.
			 *
			 *  The bank size must indicate the maximum packet size that the pipe can handle. Different pipe
			 *  numbers can handle different maximum packet sizes - refer to the chosen USB AVR's datasheet to
			 *  determine the maximum bank size for each pipe.
			 *
			 *  The banking mode may be either \ref PIPE_BANK_SINGLE or \ref PIPE_BANK_DOUBLE.
			 *
			 *  A newly configured pipe is frozen by default, and must be unfrozen before use via the \ref Pipe_Unfreeze()
			 *  before being used. Pipes should be kept frozen unless waiting for data from a device while in IN mode, or
			 *  sending data to the device in OUT mode. IN type pipes are also automatically configured to accept infinite
			 *  numbers of IN requests without automatic freezing - this can be overridden by a call to
			 *  \ref Pipe_SetFiniteINRequests().
			 *
			 *  \note The default control pipe does not have to be manually configured, as it is automatically
			 *  configured by the library internally.
			 *
			 *  \note This routine will select the specified pipe, and the pipe will remain selected once the
			 *        routine completes regardless of if the pipe configuration succeeds.
			 *
			 *  \return Boolean true if the configuration is successful, false otherwise
			 */
			bool Pipe_ConfigurePipe(const uint8_t  Number, const uint8_t Type, const uint8_t Token, const uint8_t EndpointNumber,
			                        const uint16_t Size, const uint8_t Banks);

			/** Spinloops until the currently selected non-control pipe is ready for the next packed of data to be read 
			 *  or written to it, aborting in the case of an error condition (such as a timeout or device disconnect).
			 *
			 *  \ingroup Group_PipeRW
			 *
			 *  \return A value from the Pipe_WaitUntilReady_ErrorCodes_t enum.
			 */
			uint8_t Pipe_WaitUntilReady(void);
			
			/** Determines if a pipe has been bound to the given device endpoint address. If a pipe which is bound to the given
			 *  endpoint is found, it is automatically selected.
			 *
			 *  \param EndpointAddress Address of the endpoint within the attached device to check
			 *
			 *  \return Boolean true if a pipe bound to the given endpoint address is found, false otherwise
			 */
			bool Pipe_IsEndpointBound(uint8_t EndpointAddress);
		
			/** Reads and discards the given number of bytes from the pipe, discarding fully read packets from the host
			 *  as needed. The last packet is not automatically discarded once the remaining bytes has been read; the
			 *  user is responsible for manually discarding the last packet from the device via the \ref Pipe_ClearIN() macro.
			 *  Between each USB packet, the given stream callback function is executed repeatedly until the next packet is ready,
			 *  allowing for early aborts of stream transfers.
			 *
			 *	The callback routine should be created according to the information in \ref Group_StreamCallbacks.
			 *  If the token NO_STREAM_CALLBACKS is passed via the -D option to the compiler, stream callbacks are
			 *  disabled and this function has the Callback parameter omitted.
			 *
			 *  The pipe token is set automatically, thus this can be used on bi-directional pipes directly without
			 *  having to explicitly change the data direction with a call to \ref Pipe_SetPipeToken().
			 *
			 *  \ingroup Group_PipeStreamRW
			 *
			 *  \param[in] Length  Number of bytes to send via the currently selected pipe.
			 *  \param[in] Callback  Name of a callback routine to call between successive USB packet transfers, NULL if no callback
			 *
			 *  \return A value from the \ref Pipe_Stream_RW_ErrorCodes_t enum.
			 */
			uint8_t Pipe_Discard_Stream(uint16_t Length _CALLBACK_PARAM);

			/** Writes the given number of bytes to the pipe from the given buffer in little endian,
			 *  sending full packets to the device as needed. The last packet filled is not automatically sent;
			 *  the user is responsible for manually sending the last written packet to the host via the
			 *  \ref Pipe_ClearOUT() macro. Between each USB packet, the given stream callback function is
			 *  executed repeatedly until the next packet is ready, allowing for early aborts of stream transfers.
			 *
			 *	The callback routine should be created according to the information in \ref Group_StreamCallbacks.
			 *  If the token NO_STREAM_CALLBACKS is passed via the -D option to the compiler, stream callbacks are
			 *  disabled and this function has the Callback parameter omitted.
			 *
			 *  The pipe token is set automatically, thus this can be used on bi-directional pipes directly without
			 *  having to explicitly change the data direction with a call to \ref Pipe_SetPipeToken().
			 *
			 *  \ingroup Group_PipeStreamRW
			 *
			 *  \param[in] Buffer    Pointer to the source data buffer to read from.
			 *  \param[in] Length    Number of bytes to read for the currently selected pipe into the buffer.
			 *  \param[in] Callback  Name of a callback routine to call between successive USB packet transfers, NULL if no callback
			 *
			 *  \return A value from the \ref Pipe_Stream_RW_ErrorCodes_t enum.
			 */
			uint8_t Pipe_Write_Stream_LE(void* Buffer, uint16_t Length _CALLBACK_PARAM) ATTR_NON_NULL_PTR_ARG(1);				

			/** EEPROM buffer source version of \ref Pipe_Write_Stream_LE.
			 *
			 *  \ingroup Group_PipeStreamRW
			 *
			 *  \param[in] Buffer    Pointer to the source data buffer to read from.
			 *  \param[in] Length    Number of bytes to read for the currently selected pipe into the buffer.
			 *  \param[in] Callback  Name of a callback routine to call between successive USB packet transfers, NULL if no callback
			 *
			 *  \return A value from the \ref Pipe_Stream_RW_ErrorCodes_t enum.
			 */
			uint8_t Pipe_Write_EStream_LE(void* Buffer, uint16_t Length _CALLBACK_PARAM) ATTR_NON_NULL_PTR_ARG(1);
			
			/** FLASH buffer source version of \ref Pipe_Write_Stream_LE.
			 *
			 *  \note The FLASH data must be located in the first 64KB of FLASH for this function to work correctly.
			 *
			 *  \ingroup Group_PipeStreamRW
			 *
			 *  \param[in] Buffer    Pointer to the source data buffer to read from.
			 *  \param[in] Length    Number of bytes to read for the currently selected pipe into the buffer.
			 *  \param[in] Callback  Name of a callback routine to call between successive USB packet transfers, NULL if no callback
			 *
			 *  \return A value from the \ref Pipe_Stream_RW_ErrorCodes_t enum.
			 */
			uint8_t Pipe_Write_PStream_LE(void* Buffer, uint16_t Length _CALLBACK_PARAM) ATTR_NON_NULL_PTR_ARG(1);
						
			/** Writes the given number of bytes to the pipe from the given buffer in big endian,
			 *  sending full packets to the device as needed. The last packet filled is not automatically sent;
			 *  the user is responsible for manually sending the last written packet to the host via the
			 *  \ref Pipe_ClearOUT() macro. Between each USB packet, the given stream callback function is
			 *  executed repeatedly until the next packet is ready, allowing for early aborts of stream transfers.
			 *
			 *	The callback routine should be created according to the information in \ref Group_StreamCallbacks.
			 *  If the token NO_STREAM_CALLBACKS is passed via the -D option to the compiler, stream callbacks are
			 *  disabled and this function has the Callback parameter omitted.
			 *
			 *  The pipe token is set automatically, thus this can be used on bi-directional pipes directly without
			 *  having to explicitly change the data direction with a call to \ref Pipe_SetPipeToken().
			 *
			 *  \ingroup Group_PipeStreamRW
			 *
			 *  \param[in] Buffer    Pointer to the source data buffer to read from.
			 *  \param[in] Length    Number of bytes to read for the currently selected pipe into the buffer.
			 *  \param[in] Callback  Name of a callback routine to call between successive USB packet transfers, NULL if no callback
			 *
			 *  \return A value from the \ref Pipe_Stream_RW_ErrorCodes_t enum.
			 */
			uint8_t Pipe_Write_Stream_BE(void* Buffer, uint16_t Length _CALLBACK_PARAM) ATTR_NON_NULL_PTR_ARG(1);

			/** EEPROM buffer source version of \ref Pipe_Write_Stream_BE.
			 *
			 *  \ingroup Group_PipeStreamRW
			 *
			 *  \param[in] Buffer    Pointer to the source data buffer to read from.
			 *  \param[in] Length    Number of bytes to read for the currently selected pipe into the buffer.
			 *  \param[in] Callback  Name of a callback routine to call between successive USB packet transfers, NULL if no callback
			 *
			 *  \return A value from the \ref Pipe_Stream_RW_ErrorCodes_t enum.
			 */
			uint8_t Pipe_Write_EStream_BE(void* Buffer, uint16_t Length _CALLBACK_PARAM) ATTR_NON_NULL_PTR_ARG(1);
			
			/** FLASH buffer source version of \ref Pipe_Write_Stream_BE.
			 *
			 *  \note The FLASH data must be located in the first 64KB of FLASH for this function to work correctly.
			 *
			 *  \ingroup Group_PipeStreamRW
			 *
			 *  \param[in] Buffer    Pointer to the source data buffer to read from.
			 *  \param[in] Length    Number of bytes to read for the currently selected pipe into the buffer.
			 *  \param[in] Callback  Name of a callback routine to call between successive USB packet transfers, NULL if no callback
			 *
			 *  \return A value from the \ref Pipe_Stream_RW_ErrorCodes_t enum.
			 */
			uint8_t Pipe_Write_PStream_BE(void* Buffer, uint16_t Length _CALLBACK_PARAM) ATTR_NON_NULL_PTR_ARG(1);

			/** Reads the given number of bytes from the pipe into the given buffer in little endian,
			 *  sending full packets to the device as needed. The last packet filled is not automatically sent;
			 *  the user is responsible for manually sending the last written packet to the host via the
			 *  \ref Pipe_ClearIN() macro. Between each USB packet, the given stream callback function is
			 *  executed repeatedly until the next packet is ready, allowing for early aborts of stream transfers.
			 *
			 *	The callback routine should be created according to the information in \ref Group_StreamCallbacks.
			 *  If the token NO_STREAM_CALLBACKS is passed via the -D option to the compiler, stream callbacks are
			 *  disabled and this function has the Callback parameter omitted.
			 *
			 *  The pipe token is set automatically, thus this can be used on bi-directional pipes directly without
			 *  having to explicitly change the data direction with a call to \ref Pipe_SetPipeToken().
			 *
			 *  \ingroup Group_PipeStreamRW
			 *
			 *  \param[out] Buffer    Pointer to the source data buffer to write to.
			 *  \param[in] Length    Number of bytes to read for the currently selected pipe to read from.
			 *  \param[in] Callback  Name of a callback routine to call between successive USB packet transfers, NULL if no callback
			 *
			 *  \return A value from the \ref Pipe_Stream_RW_ErrorCodes_t enum.
			 */
			uint8_t Pipe_Read_Stream_LE(void* Buffer, uint16_t Length _CALLBACK_PARAM) ATTR_NON_NULL_PTR_ARG(1);

			/** EEPROM buffer source version of \ref Pipe_Read_Stream_LE.
			 *
			 *  \ingroup Group_PipeStreamRW
			 *
			 *  \param[out] Buffer    Pointer to the source data buffer to write to.
			 *  \param[in] Length    Number of bytes to read for the currently selected pipe to read from.
			 *  \param[in] Callback  Name of a callback routine to call between successive USB packet transfers, NULL if no callback
			 *
			 *  \return A value from the \ref Pipe_Stream_RW_ErrorCodes_t enum.
			 */
			uint8_t Pipe_Read_EStream_LE(void* Buffer, uint16_t Length _CALLBACK_PARAM) ATTR_NON_NULL_PTR_ARG(1);

			/** Reads the given number of bytes from the pipe into the given buffer in big endian,
			 *  sending full packets to the device as needed. The last packet filled is not automatically sent;
			 *  the user is responsible for manually sending the last written packet to the host via the
			 *  \ref Pipe_ClearIN() macro. Between each USB packet, the given stream callback function is
			 *  executed repeatedly until the next packet is ready, allowing for early aborts of stream transfers.
			 *
			 *	The callback routine should be created according to the information in \ref Group_StreamCallbacks.
			 *  If the token NO_STREAM_CALLBACKS is passed via the -D option to the compiler, stream callbacks are
			 *  disabled and this function has the Callback parameter omitted.
			 *
			 *  The pipe token is set automatically, thus this can be used on bi-directional pipes directly without
			 *  having to explicitly change the data direction with a call to \ref Pipe_SetPipeToken().
			 *
			 *  \ingroup Group_PipeStreamRW
			 *
			 *  \param[out] Buffer    Pointer to the source data buffer to write to.
			 *  \param[in] Length    Number of bytes to read for the currently selected pipe to read from.
			 *  \param[in] Callback  Name of a callback routine to call between successive USB packet transfers, NULL if no callback
			 *
			 *  \return A value from the \ref Pipe_Stream_RW_ErrorCodes_t enum.
			 */
			uint8_t Pipe_Read_Stream_BE(void* Buffer, uint16_t Length _CALLBACK_PARAM) ATTR_NON_NULL_PTR_ARG(1);
			
			/** EEPROM buffer source version of \ref Pipe_Read_Stream_BE.
			 *
			 *  \ingroup Group_PipeStreamRW
			 *
			 *  \param[out] Buffer    Pointer to the source data buffer to write to.
			 *  \param[in] Length    Number of bytes to read for the currently selected pipe to read from.
			 *  \param[in] Callback  Name of a callback routine to call between successive USB packet transfers, NULL if no callback
			 *
			 *  \return A value from the \ref Pipe_Stream_RW_ErrorCodes_t enum.
			 */
			uint8_t Pipe_Read_EStream_BE(void* Buffer, uint16_t Length _CALLBACK_PARAM) ATTR_NON_NULL_PTR_ARG(1);

	/* Private Interface - For use in library only: */
	#if !defined(__DOXYGEN__)
		/* Macros: */
			#define PIPE_TOKEN_MASK                (0x03 << PTOKEN0)

			#if !defined(ENDPOINT_CONTROLEP)
				#define ENDPOINT_CONTROLEP         0
			#endif
			
			#define Pipe_AllocateMemory()          MACROS{ UPCFG1X |=  (1 << ALLOC); }MACROE
			#define Pipe_DeallocateMemory()        MACROS{ UPCFG1X &= ~(1 << ALLOC); }MACROE

		/* Function Prototypes: */
			void Pipe_ClearPipes(void);

		/* Inline Functions: */
			static inline uint8_t Pipe_BytesToEPSizeMask(uint16_t Bytes) ATTR_WARN_UNUSED_RESULT ATTR_CONST ATTR_ALWAYS_INLINE;
			static inline uint8_t Pipe_BytesToEPSizeMask(uint16_t Bytes)
			{
				if (Bytes <= 8)
				  return (0 << EPSIZE0);
				else if (Bytes <= 16)
				  return (1 << EPSIZE0);
				else if (Bytes <= 32)
				  return (2 << EPSIZE0);
				else if (Bytes <= 64)
				  return (3 << EPSIZE0);
				else if (Bytes <= 128)
				  return (4 << EPSIZE0);
				else
				  return (5 << EPSIZE0);
			};

	#endif

	/* Disable C linkage for C++ Compilers: */
		#if defined(__cplusplus)
			}
		#endif
	
#endif

/** @} */
