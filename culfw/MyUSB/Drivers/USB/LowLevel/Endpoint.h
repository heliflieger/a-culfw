/*
             MyUSB Library
     Copyright (C) Dean Camera, 2008.
              
  dean [at] fourwalledcubicle [dot] com
      www.fourwalledcubicle.com
*/

/*
  Copyright 2008  Dean Camera (dean [at] fourwalledcubicle [dot] com)

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

#ifndef __ENDPOINT_H__
#define __ENDPOINT_H__

	/* Includes: */
		#include <avr/io.h>
		#include <stdbool.h>

		#include "../../../Common/Common.h"
		#include "../HighLevel/USBTask.h"

	/* Enable C linkage for C++ Compilers: */
		#if defined(__cplusplus)
			extern "C" {
		#endif

	/* Public Interface - May be used in end-application: */
		/* Macros: */
			/** Endpoint data direction mask for Endpoint_ConfigureEndpoint(). This indicates that the endpoint
			 *  should be initialized in the OUT direction - i.e. data flows from host to device.
			 */
			#define ENDPOINT_DIR_OUT                           0

			/** Endpoint data direction mask for Endpoint_ConfigureEndpoint(). This indicates that the endpoint
			 *  should be initialized in the IN direction - i.e. data flows from device to host.
			 */
			#define ENDPOINT_DIR_IN                            (1 << EPDIR)

			/** Mask for the bank mode selection for the Endpoint_ConfigureEndpoint() macro. This indicates
			 *  that the endpoint should have one single bank, which requires less USB FIFO memory but results
			 *  in slower transfers as only one USB device (the AVR or the host) can access the endpoint's
			 *  bank at the one time.
			 */
			#define ENDPOINT_BANK_SINGLE                       0

			/** Mask for the bank mode selection for the Endpoint_ConfigureEndpoint() macro. This indicates
			 *  that the endpoint should have two banks, which requires more USB FIFO memory but results
			 *  in faster transfers as one USB device (the AVR or the host) can access one bank while the other
			 *  accesses the second bank.
			 */
			#define ENDPOINT_BANK_DOUBLE                       (1 << EPBK0)
			
			/** Endpoint address for the default control endpoint, which always resides in address 0. This is
			 *  defined for convenience to give more readable code when used with the endpoint macros.
			 */
			#define ENDPOINT_CONTROLEP                         0

			/** Default size of the default control endpoint's bank, until altered by the Endpoint0Size value 
			 *  in the device descriptor.
			 */
			#define ENDPOINT_CONTROLEP_DEFAULT_SIZE            8
			
			/** Endpoint number mask, for masking against endpoint addresses to retrieve the endpoint's
			 *  numerical address in the device.
			 */
			#define ENDPOINT_EPNUM_MASK                        0b111

			/** Endpoint bank size mask, for masking against endpoint addresses to retrieve the endpoint's
			 *  bank size in the device.
			 */
			#define ENDPOINT_EPSIZE_MASK                       0x7FF
			
			#if defined(USB_FULL_CONTROLLER) || defined(USB_MODIFIED_FULL_CONTROLLER) || defined(__DOXYGEN__)
				/** Total number of endpoints (including the default control endpoint at address 0) which may
				 *  be used in the device. Different USB AVR models support different amounts of endpoints,
				 *  this value reflects the maximum number of endpoints for the currently selected AVR model.
				 */
				#define ENDPOINT_MAX_ENDPOINTS                  7

				/** Size in bytes of the largest endpoint bank size possible in the device. Not all banks on
				 *  each AVR model supports the largest bank size possible on the device; different endpoint
				 *  numbers support different maximum bank sizes. This value reflects the largest possible
				 *  bank of any endpoint on the currently selected USB AVR model.
				 */
				#define ENDPOINT_MAX_SIZE                       256
			#else
				#define ENDPOINT_MAX_ENDPOINTS                  5			
				#define ENDPOINT_MAX_SIZE                       64			
			#endif

			/** Interrupt definition for the endpoint SETUP interrupt (for CONTROL type endpoints). Should be
			 *  used with the USB_INT_* macros located in USBInterrupt.h.
			 *
			 *  This interrupt will fire if enabled on a CONTROL type endpoint if a new control packet is
			 *  received from the host.
			 */
			#define ENDPOINT_INT_SETUP                         UEIENX, (1 << RXSTPE), UEINTX, (1 << RXSTPI)

			/** Interrupt definition for the endpoint IN interrupt (for INTERRUPT type endpoints). Should be
			 *  used with the USB_INT_* macros located in USBInterrupt.h.
			 *
			 *  This interrupt will fire if enabled on an INTERRUPT type endpoint if a the endpoint interrupt
			 *  period has elapsed and the endpoint is ready for a new packet to be written to its FIFO buffer
			 *  (if required).
			 */
			#define ENDPOINT_INT_IN                            UEIENX, (1 << TXINE) , UEINTX, (1 << TXINI)

			/** Interrupt definition for the endpoint OUT interrupt (for INTERRUPT type endpoints). Should be
			 *  used with the USB_INT_* macros located in USBInterrupt.h.
			 *
			 *  This interrupt will fire if enabled on an INTERRUPT type endpoint if a the endpoint interrupt
			 *  period has elapsed and the endpoint is ready for a packet from the host to be read from its
			 *  FIFO buffer (if received).
			 */
			#define ENDPOINT_INT_OUT                           UEIENX, (1 << RXOUTE), UEINTX, (1 << RXOUTI)
			
			#if defined(USB_FULL_CONTROLLER) || defined(USB_MODIFIED_FULL_CONTROLLER) || defined(__DOXYGEN__)
				/** Indicates the number of bytes currently stored in the current endpoint's selected bank. */
				#define Endpoint_BytesInEndpoint()             UEBCX
			#else
				#define Endpoint_BytesInEndpoint()             UEBCLX
			#endif
			
			/** Returns the endpoint address of the currently selected endpoint. This is typically used to save
			 *  the currently selected endpoint number so that it can be restored after another endpoint has
			 *  been manipulated.
			 */
			#define Endpoint_GetCurrentEndpoint()             (UENUM   &   ENDPOINT_EPNUM_MASK)
			
			/** Selects the given endpoint number. If the address from the device descriptors is used, the
			 *  value should be masked with the ENDPOINT_EPNUM_MASK constant to extract only the endpoint
			 *  number (and discarding the endpoint direction bit).
			 *
			 *  Any endpoint operations which do not require the endpoint number to be indicated will operate on
			 *  the currently selected endpoint.
			 */
			#define Endpoint_SelectEndpoint(epnum)     MACROS{ UENUM    =  epnum;                         }MACROE

			/** Resets the endpoint bank FIFO. This clears all the endpoint banks and resets the USB controller's
			 *  In and Out pointers to the bank's contents.
			 */
			#define Endpoint_ResetFIFO(epnum)          MACROS{ UERST    =  (1 << epnum); UERST = 0;       }MACROE

			/** Enables the currently selected endpoint so that data can be sent and received through it to
			 *  and from a host.
			 *
			 *  \note Endpoints must first be configured properly rather than just being enabled via the
			 *        Endpoint_ConfigureEndpoint() macro, which calls Endpoint_EnableEndpoint() automatically.
			 */
			#define Endpoint_EnableEndpoint()          MACROS{ UECONX  |=  (1 << EPEN);                   }MACROE

			/** Disables the currently selected endpoint so that data cannot be sent and received through it
			 *  to and from a host.
			 */
			#define Endpoint_DisableEndpoint()         MACROS{ UECONX  &= ~(1 << EPEN);                   }MACROE

			/** Returns true if the currently selected endpoint is enabled, false otherwise. */
			#define Endpoint_IsEnabled()                     ((UECONX  &   (1 << EPEN)) ? true : false)

			/** Returns true if the currently selected endpoint may be read from (if data is waiting in the endpoint
			 *  bank and the endpoint is an OUT direction, or if the bank is not yet full if the endpoint is an
			 *  IN direction). This function will return false if an error has occured in the endpoint, or if
			 *  the endpoint is an OUT direction and no packet has been received, or if the endpoint is an IN
			 *  direction and the endpoint bank is full.
			 */
			#define Endpoint_ReadWriteAllowed()              ((UEINTX  & (1 << RWAL)) ? true : false)

			/** Configures the specified endpoint number with the given endpoint type, direction, bank size
			 *  and banking mode. Endpoints should be allocated in ascending order by their address in the
			 *  device (i.e. endpoint 1 should be configured before endpoint 2 and so on).
			 *
			 *  The endpoint type may be one of the EP_TYPE_* macros listed in LowLevel.h and the direction
			 *  may be either ENDPOINT_DIR_OUT or ENDPOINT_DIR_IN.
			 *
			 *  The bank size must indicate the maximum packet size that the endpoint can handle. Different
			 *  endpoint numbers can handle different maximum packet sizes - refer to the chosen USB AVR's
			 *  datasheet to determine the maximum bank size for each endpoint.
			 *
			 *  The banking mode may be either ENDPOINT_BANK_SINGLE or ENDPOINT_BANK_DOUBLE.
			 *
			 *  The success of this routine can be determined via the Endpoint_IsConfigured() macro.
			 *
			 *  \note This routine will select the specified endpoint, and the endpoint will remain selected
			 *        once the routine completes regardless of if the endpoint configuration succeeds.
			 */
			#define Endpoint_ConfigureEndpoint(num, type, dir, size, banks)                           \
												       Endpoint_ConfigureEndpoint_P(num, size,        \
                                                       ((type << EPTYPE0) | dir),                     \
											           banks)
													   
			/** Returns true if the currently selected endpoint is configured, false otherwise. */
			#define Endpoint_IsConfigured()                  ((UESTA0X & (1 << CFGOK)) ? true : false)

			/** Returns a mask indicating which INTERRUPT type endpoints have interrupted - i.e. their
			 *  interrupt duration has elapsed. Which endpoints have interrupted can be determined by
			 *  masking the return value against (1 << {Endpoint Number}).
			 */
			#define Endpoint_GetEndpointInterrupts()           UEINT

			/** Clears the endpoint interrupt flag. This clears the specified endpoint number's interrupt
			 *  mask in the endpoint interrupt flag register.
			 */
			#define Endpoint_ClearEndpointInterrupt(n) MACROS{ UEINT   &= ~(1 << n);                     }MACROE

			/** Returns true if the specified endpoint number has interrupted (valid only for INTERRUPT type
			 *  endpoints), false otherwise.
			 */
			#define Endpoint_HasEndpointInterrupted(n)       ((UEINT   &   (1 << n)) ? true : false)

			/** Clears the currently selected endpoint bank, and switches to the alternate bank if the currently
			 *  selected endpoint is dual-banked. When cleared, this either frees the bank up for the next packet
			 *  from the host (if the endpoint is of the OUT direction) or sends the packet contents to the host
			 *  (if the endpoint is of the IN direction).
			 */
			#define Endpoint_ClearCurrentBank()       MACROS{ UEINTX  &= ~(1 << FIFOCON);               }MACROE
			
			/** Returns true if the current CONTROL type endpoint is ready for an IN packet, false otherwise. */
			#define Endpoint_IsSetupINReady()               ((UEINTX  & (1 << TXINI))  ? true : false)

			/** Returns true if the current CONTROL type endpoint is ready for an OUT packet, false otherwise. */
			#define Endpoint_IsSetupOUTReceived()           ((UEINTX  & (1 << RXOUTI)) ? true : false)

			/** Returns true if the current CONTROL type endpoint is ready for a SETUP packet, false otherwise. */
			#define Endpoint_IsSetupReceived()              ((UEINTX  & (1 << RXSTPI)) ? true : false)

			/** Clears a received SETUP packet on the currently selected CONTROL type endpoint. */
			#define Endpoint_ClearSetupReceived()      MACROS{ UEINTX  &= ~(1 << RXSTPI);                }MACROE

			/** Sends an IN packet to the host on the currently selected CONTROL type endpoint. */
			#define Endpoint_ClearSetupIN()            MACROS{ UEINTX  &= ~(1 << TXINI);                 }MACROE

			/** Acknowedges an OUT packet to the host on the currently selected CONTROL type endpoint, freeing
			 *  up the endpoint for the next packet.
			 */
			#define Endpoint_ClearSetupOUT()           MACROS{ UEINTX  &= ~(1 << RXOUTI);                }MACROE

			/** Stalls the current endpoint, indicating to the host that a logical problem occured with the
			 *  indicated endpoint and that the current transfer sequence should be aborted. This provides a
			 *  way for devices to indicate invalid commands to the host so that the current transfer can be
			 *  aborted and the host can begin its own recovery seqeuence.
			 *
			 *  The currently selected endpoint remains stalled until either the Endpoint_ClearStall() macro
			 *  is called, or the host issues a CLEAR FEATURE request to the device for the currently selected
			 *  endpoint.
			 */
			#define Endpoint_StallTransaction()        MACROS{ UECONX  |=  (1 << STALLRQ);               }MACROE

			/** Clears the stall on the currently selected endpoint. */
			#define Endpoint_ClearStall()              MACROS{ UECONX  |=  (1 << STALLRQC);              }MACROE

			/** Returns true if the currently selected endpoint is stalled, false othewise. */
			#define Endpoint_IsStalled()                     ((UECONX  &   (1 << STALLRQ)) ? true : false)

			/** Resets the data toggle of the currently selected endpoint. */
			#define Endpoint_ResetDataToggle()         MACROS{ UECONX  |=  (1 << RSTDT);                 }MACROE

		/* Enums: */
			/** Enum for the possible error return codes of the Endpoint_*_Stream_* functions. */
			enum Endpoint_Stream_RW_ErrorCodes_t
			{
				ENDPOINT_RWSTREAM_ERROR_NoError            = 0, /**< Command completed successfully, no error. */
				ENDPOINT_RWSTREAM_ERROR_EndpointStalled    = 1,	/**< The host stalled the pipe during the transfer. */
				ENDPOINT_RWSTREAM_ERROR_DeviceDisconnected = 2, /**< Device was disconnected from the host during
				                                                 *   the transfer.
				                                                 */
			};
			
			/** Enum for the possible error return codes of the Endpoint_*_Control_Stream_* functions. */
			enum Endpoint_ControlStream_RW_ErrorCodes_t
			{
				ENDPOINT_RWCSTREAM_ERROR_NoError            = 0, /**< Command completed successfully, no error. */
				ENDPOINT_RWCSTREAM_ERROR_HostAborted        = 1, /**< The aborted the transfer prematurely. */
			};

		/* Inline Functions: */
			/** Reads one byte from the currently selected endpoint's bank, for OUT direction endpoints. */
			static inline uint8_t Endpoint_Read_Byte(void) ATTR_WARN_UNUSED_RESULT;
			static inline uint8_t Endpoint_Read_Byte(void)
			{
				return UEDATX;
			}

			/** Writes one byte from the currently selected endpoint's bank, for IN direction endpoints. */
			static inline void Endpoint_Write_Byte(const uint8_t Byte)
			{
				UEDATX = Byte;
			}

			/** Discards one byte from the currently selected endpoint's bank, for OUT direction endpoints. */
			static inline void Endpoint_Discard_Byte(void)
			{
				uint8_t Dummy;
				
				Dummy = UEDATX;
			}
			
			/** Reads two bytes from the currently selected endpoint's bank in little endian format, for OUT
			 *  direction endpoints.
			 */
			static inline uint16_t Endpoint_Read_Word_LE(void) ATTR_WARN_UNUSED_RESULT;
			static inline uint16_t Endpoint_Read_Word_LE(void)
			{
				uint16_t Data;
				
				Data  = UEDATX;
				Data |= (((uint16_t)UEDATX) << 8);
			
				return Data;
			}

			/** Reads two bytes from the currently selected endpoint's bank in big endian format, for OUT
			 *  direction endpoints.
			 */
			static inline uint16_t Endpoint_Read_Word_BE(void) ATTR_WARN_UNUSED_RESULT;
			static inline uint16_t Endpoint_Read_Word_BE(void)
			{
				uint16_t Data;
				
				Data  = (((uint16_t)UEDATX) << 8);
				Data |= UEDATX;
			
				return Data;
			}

			/** Writes two bytes to the currently selected endpoint's bank in little endian format, for IN
			 *  direction endpoints.
			 */
			static inline void Endpoint_Write_Word_LE(const uint16_t Word)
			{
				UEDATX = (Word & 0xFF);
				UEDATX = (Word >> 8);
			}
			
			/** Writes two bytes to the currently selected endpoint's bank in big endian format, for IN
			 *  direction endpoints.
			 */
			static inline void Endpoint_Write_Word_BE(const uint16_t Word)
			{
				UEDATX = (Word >> 8);
				UEDATX = (Word & 0xFF);
			}

			/** Discards two bytes from the currently selected endpoint's bank, for OUT direction endpoints. */
			static inline void Endpoint_Discard_Word(void)
			{
				uint8_t Dummy;
				
				Dummy = UEDATX;
				Dummy = UEDATX;
			}

			/** Reads four bytes from the currently selected endpoint's bank in little endian format, for OUT
			 *  direction endpoints.
			 */
			static inline uint32_t Endpoint_Read_DWord_LE(void) ATTR_WARN_UNUSED_RESULT;
			static inline uint32_t Endpoint_Read_DWord_LE(void)
			{
				union
				{
					uint32_t DWord;
					uint8_t  Bytes[4];
				} Data;
				
				Data.Bytes[0] = UEDATX;
				Data.Bytes[1] = UEDATX;
				Data.Bytes[2] = UEDATX;
				Data.Bytes[3] = UEDATX;
			
				return Data.DWord;
			}

			/** Reads four bytes from the currently selected endpoint's bank in big endian format, for OUT
			 *  direction endpoints.
			 */
			static inline uint32_t Endpoint_Read_DWord_BE(void) ATTR_WARN_UNUSED_RESULT;
			static inline uint32_t Endpoint_Read_DWord_BE(void)
			{
				union
				{
					uint32_t DWord;
					uint8_t  Bytes[4];
				} Data;
				
				Data.Bytes[3] = UEDATX;
				Data.Bytes[2] = UEDATX;
				Data.Bytes[1] = UEDATX;
				Data.Bytes[0] = UEDATX;
			
				return Data.DWord;
			}

			/** Writes four bytes to the currently selected endpoint's bank in little endian format, for IN
			 *  direction endpoints.
			 */
			static inline void Endpoint_Write_DWord_LE(const uint32_t DWord)
			{
				Endpoint_Write_Word_LE(DWord);
				Endpoint_Write_Word_LE(DWord >> 16);
			}
			
			/** Writes four bytes to the currently selected endpoint's bank in big endian format, for IN
			 *  direction endpoints.
			 */
			static inline void Endpoint_Write_DWord_BE(const uint32_t DWord)
			{
				Endpoint_Write_Word_BE(DWord >> 16);
				Endpoint_Write_Word_BE(DWord);
			}

			/** Discards four bytes from the currently selected endpoint's bank, for OUT direction endpoints. */
			static inline void Endpoint_Discard_DWord(void)
			{
				uint8_t Dummy;
				
				Dummy = UEDATX;
				Dummy = UEDATX;
				Dummy = UEDATX;
				Dummy = UEDATX;
			}

		/* External Variables: */
			/** Global indicating the maximum packet size of the default control endpoint located at address
			 *  0 in the device. This value is set to the value indicated in the device descriptor in the user
			 *  project once the USB interface is initialized into device mode.
			 *
			 *  \note This variable should be treated as read-only in the user application, and never manually
			 *        changed in value.
			 */
			extern uint8_t USB_ControlEndpointSize;

		/* Function Prototypes: */
			/** Writes the given number of bytes to the endpoint from the given buffer in little endian,
			 *  sending full packets to the host as needed. The last packet filled is not automatically sent;
			 *  the user is responsible for manually sending the last written packet to the host via the
			 *  Endpoint_ClearCurrentBank() macro.
			 *
			 *  \note This routine should not be used on CONTROL type endpoints.
			 *
			 *  \param Buffer  Pointer to the buffer to write the received bytes to.
			 *  \param Length  Number of bytes to read for the currently selected endpoint into the buffer.
			 *
			 *  \return A value from the Endpoint_Stream_RW_ErrorCodes_t enum.
			 */
			uint8_t Endpoint_Write_Stream_LE(const void* Buffer, uint16_t Length) ATTR_NON_NULL_PTR_ARG(1);

			/** Writes the given number of bytes to the endpoint from the given buffer in big endian,
			 *  sending full packets to the host as needed. The last packet filled is not automatically sent;
			 *  the user is responsible for manually sending the last written packet to the host via the
			 *  Endpoint_ClearCurrentBank() macro.
			 *
			 *  \note This routine should not be used on CONTROL type endpoints.
			 *
			 *  \param Buffer  Pointer to the buffer to write the received bytes to.
			 *  \param Length  Number of bytes to read for the currently selected endpoint into the buffer.
			 *
			 *  \return A value from the Endpoint_Stream_RW_ErrorCodes_t enum.
			 */
			uint8_t Endpoint_Write_Stream_BE(const void* Buffer, uint16_t Length) ATTR_NON_NULL_PTR_ARG(1);

			/** Reads the given number of bytes from the endpoint from the given buffer in little endian,
			 *  discarding fully read packets from the host as needed. The last packet is not automatically
			 *  discarded once the remaining bytes has been read; the user is responsible for manually
			 *  discarding the last packet from the host via the Endpoint_ClearCurrentBank() macro.
			 *
			 *  \note This routine should not be used on CONTROL type endpoints.
			 *
			 *  \param Buffer  Pointer to the buffer to read the bytes to send from.
			 *  \param Length  Number of bytes to send via the currently selected endpoint.
			 *
			 *  \return A value from the Endpoint_Stream_RW_ErrorCodes_t enum.
			 */
			uint8_t Endpoint_Read_Stream_LE(void* Buffer, uint16_t Length)  ATTR_NON_NULL_PTR_ARG(1);

			/** Reads the given number of bytes from the endpoint from the given buffer in big endian,
			 *  discarding fully read packets from the host as needed. The last packet is not automatically
			 *  discarded once the remaining bytes has been read; the user is responsible for manually
			 *  discarding the last packet from the host via the Endpoint_ClearCurrentBank() macro.
			 *
			 *  \note This routine should not be used on CONTROL type endpoints.
			 *
			 *  \param Buffer  Pointer to the buffer to read the bytes to send from.
			 *  \param Length  Number of bytes to send via the currently selected endpoint.
			 *
			 *  \return A value from the Endpoint_Stream_RW_ErrorCodes_t enum.
			 */
			uint8_t Endpoint_Read_Stream_BE(void* Buffer, uint16_t Length)  ATTR_NON_NULL_PTR_ARG(1);

			/** Writes the given number of bytes to the CONTROL type endpoint from the given buffer in little endian,
			 *  sending full packets to the host as needed. The host OUT acknowedgement is not automatically cleared
			 *  in both failure and success states; the user is responsible for manually clearing the setup OUT to
			 *  finalize the transfer via the Endpoint_ClearSetupOUT() macro.
			 *
			 *  \note This routine should only be used on CONTROL type endpoints.
			 *
			 *  \warning Unlike the standard stream read/write commands, the control stream commands cannot be chained
			 *           together; i.e. the entire stream data must be read or written at the one time.
			 *
			 *  \param Buffer  Pointer to the buffer to write the received bytes to.
			 *  \param Length  Number of bytes to read for the currently selected endpoint into the buffer.
			 *
			 *  \return A value from the Endpoint_ControlStream_RW_ErrorCodes_t enum.
			 */
			uint8_t Endpoint_Write_Control_Stream_LE(const void* Buffer, uint16_t Length) ATTR_NON_NULL_PTR_ARG(1);

			/** Writes the given number of bytes to the CONTROL type endpoint from the given buffer in big endian,
			 *  sending full packets to the host as needed. The host OUT acknowedgement is not automatically cleared
			 *  in both failure and success states; the user is responsible for manually clearing the setup OUT to
			 *  finalize the transfer via the Endpoint_ClearSetupOUT() macro.
			 *
			 *  \note This routine should only be used on CONTROL type endpoints.
			 *
			 *  \warning Unlike the standard stream read/write commands, the control stream commands cannot be chained
			 *           together; i.e. the entire stream data must be read or written at the one time.
			 *
			 *  \param Buffer  Pointer to the buffer to write the received bytes to.
			 *  \param Length  Number of bytes to read for the currently selected endpoint into the buffer.
			 *
			 *  \return A value from the Endpoint_ControlStream_RW_ErrorCodes_t enum.
			 */
			uint8_t Endpoint_Write_Control_Stream_BE(const void* Buffer, uint16_t Length) ATTR_NON_NULL_PTR_ARG(1);

			/** Reads the given number of bytes from the CONTROL endpoint from the given buffer in little endian,
			 *  discarding fully read packets from the host as needed. The device IN acknowedgement is not
			 *  automatically sent after success or failure states; the user is responsible for manually sending the
			 *  setup IN to finalize the transfer via the Endpoint_ClearSetupIN() macro.
			 *
			 *  \note This routine should only be used on CONTROL type endpoints.
			 *
			 *  \warning Unlike the standard stream read/write commands, the control stream commands cannot be chained
			 *           together; i.e. the entire stream data must be read or written at the one time.
			 *
			 *  \param Buffer  Pointer to the buffer to read the bytes to send from.
			 *  \param Length  Number of bytes to send via the currently selected endpoint.
			 *
			 *  \return A value from the Endpoint_ControlStream_RW_ErrorCodes_t enum.
			 */
			uint8_t Endpoint_Read_Control_Stream_LE(void* Buffer, uint16_t Length)  ATTR_NON_NULL_PTR_ARG(1);

			/** Reads the given number of bytes from the CONTROL endpoint from the given buffer in big endian,
			 *  discarding fully read packets from the host as needed. The device IN acknowedgement is not
			 *  automatically sent after success or failure states; the user is responsible for manually sending the
			 *  setup IN to finalize the transfer via the Endpoint_ClearSetupIN() macro.
			 *
			 *  \note This routine should only be used on CONTROL type endpoints.
			 *
			 *  \warning Unlike the standard stream read/write commands, the control stream commands cannot be chained
			 *           together; i.e. the entire stream data must be read or written at the one time.
			 *
			 *  \param Buffer  Pointer to the buffer to read the bytes to send from.
			 *  \param Length  Number of bytes to send via the currently selected endpoint.
			 *
			 *  \return A value from the Endpoint_ControlStream_RW_ErrorCodes_t enum.
			 */
			uint8_t Endpoint_Read_Control_Stream_BE(void* Buffer, uint16_t Length)  ATTR_NON_NULL_PTR_ARG(1);

		/* Function Aliases: */
			/** Alias for Endpoint_Discard_Byte().
			 */
			#define Endpoint_Ignore_Byte()                      Endpoint_Discard_Byte()

			/** Alias for Endpoint_Discard_Word().
			 */
			#define Endpoint_Ignore_Word()                      Endpoint_Discard_Word()		

			/** Alias for Endpoint_Discard_DWord().
			 */
			#define Endpoint_Ignore_DWord()                     Endpoint_Discard_DWord()
		
			/** Alias for Endpoint_Read_Word_LE(). By default USB transfers use little endian format, thus
			 *  the command with no endianness specifier indicates little endian mode.
			 */
			#define Endpoint_Read_Word()                        Endpoint_Read_Word_LE()   

			/** Alias for Endpoint_Write_Word_LE(). By default USB transfers use little endian format, thus
			 *  the command with no endianness specifier indicates little endian mode.
			 */
			#define Endpoint_Write_Word(Word)                   Endpoint_Write_Word_LE(Word)

			/** Alias for Endpoint_Read_DWord_LE(). By default USB transfers use little endian format, thus
			 *  the command with no endianness specifier indicates little endian mode.
			 */
			#define Endpoint_Read_DWord()                       Endpoint_Read_DWord_LE()

			/** Alias for Endpoint_Write_DWord_LE(). By default USB transfers use little endian format, thus
			 *  the command with no endianness specifier indicates little endian mode.
			 */
			#define Endpoint_Write_DWord(DWord)                 Endpoint_Write_DWord_LE(DWord)

			/** Alias for Endpoint_Read_Stream_LE(). By default USB transfers use little endian format, thus
			 *  the command with no endianness specifier indicates little endian mode.
			 */
			#define Endpoint_Read_Stream(Buffer, Length)        Endpoint_Read_Stream_LE(Buffer, Length)

			/** Alias for Endpoint_Write_Stream_LE(). By default USB transfers use little endian format, thus
			 *  the command with no endianness specifier indicates little endian mode.
			 */
			#define Endpoint_Write_Stream(Data, Length)         Endpoint_Write_Stream_LE(Data, Length)
			
			/** Alias for Endpoint_Read_Control_Stream_LE(). By default USB transfers use little endian format, thus
			 *  the command with no endianness specifier indicates little endian mode.
			 */
			#define Endpoint_Read_Control_Stream(Data, Length)  Endpoint_Read_Control_Stream_LE(Data, Length)

			/** Alias for Endpoint_Write_Control_Stream_LE(). By default USB transfers use little endian format, thus
			 *  the command with no endianness specifier indicates little endian mode.
			 */
			#define Endpoint_Write_Control_Stream(Data, Length) Endpoint_Write_Control_Stream_LE(Data, Length)			
			
	/* Private Interface - For use in library only: */
	#if !defined(__DOXYGEN__)
		/* Macros: */	
			#define Endpoint_AllocateMemory()          MACROS{ UECFG1X |=  (1 << ALLOC);                  }MACROE
			#define Endpoint_DeallocateMemory()        MACROS{ UECFG1X &= ~(1 << ALLOC);                  }MACROE

		/* Inline Functions: */
			static inline uint8_t Endpoint_BytesToEPSizeMask(uint16_t Bytes)
			                                                 ATTR_WARN_UNUSED_RESULT ATTR_CONST;
			static inline uint8_t Endpoint_BytesToEPSizeMask(uint16_t Bytes)
			{
				Bytes &= ENDPOINT_EPSIZE_MASK;
			
				if (Bytes <= 8)
				  return (0 << EPSIZE0);
				else if (Bytes <= 16)
				  return (1 << EPSIZE0);
				else if (Bytes <= 32)
				  return (2 << EPSIZE0);
				#if defined(USB_LIMITED_CONTROLLER)
				else
				  return (3 << EPSIZE0);
				#else
				else if (Bytes <= 64)
				  return (3 << EPSIZE0);
				else if (Bytes <= (8 << 4))
				  return (4 << EPSIZE0);
				else
				  return (5 << EPSIZE0);
				#endif
			};

		/* Function Prototypes: */
			void Endpoint_ClearEndpoints(void);
			void Endpoint_ConfigureEndpoint_P(const uint8_t  EndpointNum,
			                                  const uint16_t EndpointSize,
			                                  const uint8_t  UECFG0Xdata,
			                                  const uint8_t  UECFG1Xdata);
	#endif

	/* Disable C linkage for C++ Compilers: */
		#if defined(__cplusplus)
			}
		#endif
		
#endif
