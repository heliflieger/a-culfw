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

#include "USBMode.h"
#if defined(USB_CAN_BE_DEVICE)

#define  INCLUDE_FROM_DEVCHAPTER9_C
#include "DevChapter9.h"

uint8_t USB_ConfigurationNumber;
bool    USB_RemoteWakeupEnabled;
bool    USB_CurrentlySelfPowered;

void USB_Device_ProcessControlPacket(void)
{
	uint8_t bmRequestType  = Endpoint_Read_Byte();
	uint8_t bRequest       = Endpoint_Read_Byte();
	bool    RequestHandled = false;
	
	switch (bRequest)
	{
		case REQ_GetStatus:
			if (((bmRequestType & (CONTROL_REQTYPE_DIRECTION | CONTROL_REQTYPE_TYPE)) ==
			                            (REQDIR_DEVICETOHOST | REQTYPE_STANDARD)) &&
			    ((bmRequestType & CONTROL_REQTYPE_RECIPIENT) != REQREC_OTHER))
			{
				USB_Device_GetStatus(bmRequestType);
				RequestHandled = true;
			}

			break;
		case REQ_ClearFeature:
		case REQ_SetFeature:
			if (((bmRequestType & (CONTROL_REQTYPE_DIRECTION | CONTROL_REQTYPE_TYPE)) ==
			                            (REQDIR_HOSTTODEVICE | REQTYPE_STANDARD)) &&
			    ((bmRequestType & CONTROL_REQTYPE_RECIPIENT) != REQREC_OTHER))
			{
				USB_Device_ClearSetFeature(bRequest, bmRequestType);
				RequestHandled = true;
			}

			break;
		case REQ_SetAddress:
			if (bmRequestType == (REQDIR_HOSTTODEVICE | REQTYPE_STANDARD | REQREC_DEVICE))
			{
				USB_Device_SetAddress();
				RequestHandled = true;
			}

			break;
		case REQ_GetDescriptor:
			USB_Device_GetDescriptor();
			RequestHandled = true;
			
			break;
		case REQ_GetConfiguration:
			if (bmRequestType == (REQDIR_DEVICETOHOST | REQTYPE_STANDARD | REQREC_DEVICE))
			{
				USB_Device_GetConfiguration();
				RequestHandled = true;
			}

			break;
		case REQ_SetConfiguration:
			if (bmRequestType == (REQDIR_HOSTTODEVICE | REQTYPE_STANDARD | REQREC_DEVICE))
			{
				USB_Device_SetConfiguration();
				RequestHandled = true;
			}

			break;
	}

	if (!(RequestHandled))
	  RAISE_EVENT(USB_UnhandledControlPacket, bRequest, bmRequestType);
	  
	if (Endpoint_IsSetupReceived())
	{
		Endpoint_StallTransaction();
		Endpoint_ClearSetupReceived();		
	}
}

static void USB_Device_SetAddress(void)
{
	uint8_t wValue_LSB = Endpoint_Read_Byte();

	UDADDR = ((UDADDR & (1 << ADDEN)) | (wValue_LSB & 0x7F));

	Endpoint_ClearSetupReceived();

	Endpoint_ClearSetupIN();
	while (!(Endpoint_IsSetupINReady()));
	
	UDADDR |= (1 << ADDEN);

	return;
}

static void USB_Device_SetConfiguration(void)
{
	uint16_t                 wValue               = Endpoint_Read_Word_LE();
	bool                     NotAlreadyConfigured = !(USB_ConfigurationNumber);
	USB_Descriptor_Device_t* DevDescriptorPtr;
	uint16_t                 DevDescriptorSize;

	if ((USB_GetDescriptor((DTYPE_Device << 8), 0, (void*)&DevDescriptorPtr, &DevDescriptorSize) == false) ||
#if defined(USE_RAM_DESCRIPTORS)
	    (wValue > DevDescriptorPtr->NumberOfConfigurations))
#elif defined (USE_EEPROM_DESCRIPTORS)
	    (wValue > eeprom_read_byte(&DevDescriptorPtr->NumberOfConfigurations)))
#else
	    (wValue > pgm_read_byte(&DevDescriptorPtr->NumberOfConfigurations)))
#endif
	{
		return;
	}
	
	Endpoint_ClearSetupReceived();

	USB_ConfigurationNumber = wValue;

	Endpoint_ClearSetupIN();

	if (NotAlreadyConfigured)
	  RAISE_EVENT(USB_DeviceEnumerationComplete);

	RAISE_EVENT(USB_ConfigurationChanged);
}

void USB_Device_GetConfiguration(void)
{
	Endpoint_ClearSetupReceived();	

	Endpoint_Write_Byte(USB_ConfigurationNumber);
	
	Endpoint_ClearSetupIN();

	while (!(Endpoint_IsSetupOUTReceived()));
	Endpoint_ClearSetupOUT();
}

static void USB_Device_GetDescriptor(void)
{
	uint16_t wValue  = Endpoint_Read_Word_LE();
	uint16_t wIndex  = Endpoint_Read_Word_LE();
	uint16_t wLength = Endpoint_Read_Word_LE();
	
	void*    DescriptorPointer;
	uint16_t DescriptorSize;
	
	bool     SendZLP;
	
	if (!(USB_GetDescriptor(wValue, wIndex, &DescriptorPointer, &DescriptorSize)))
	  return;
	
	Endpoint_ClearSetupReceived();
	
	if (wLength > DescriptorSize)
	  wLength = DescriptorSize;
	  
	SendZLP = !(wLength % USB_ControlEndpointSize);
	
	while (wLength && (!(Endpoint_IsSetupOUTReceived())))
	{
		while (!(Endpoint_IsSetupINReady()));
		
		while (wLength && (Endpoint_BytesInEndpoint() < USB_ControlEndpointSize))
		{
			#if defined(USE_RAM_DESCRIPTORS)
			Endpoint_Write_Byte(*((uint8_t*)DescriptorPointer++));
			#elif defined (USE_EEPROM_DESCRIPTORS)
			Endpoint_Write_Byte(eeprom_read_byte(DescriptorPointer++));			
			#else
			Endpoint_Write_Byte(pgm_read_byte(DescriptorPointer++));
			#endif
			
			wLength--;
		}
		
		Endpoint_ClearSetupIN();
	}
	
	if (Endpoint_IsSetupOUTReceived())
	{
		Endpoint_ClearSetupOUT();
		return;
	}
	
	if (SendZLP)
	{
		while (!(Endpoint_IsSetupINReady()));
		Endpoint_ClearSetupIN();
	}

	while (!(Endpoint_IsSetupOUTReceived()));
	Endpoint_ClearSetupOUT();
}

static void USB_Device_GetStatus(const uint8_t bmRequestType)
{
	uint16_t CurrentStatus = 0;
	
	USB_Descriptor_Configuration_Header_t* ConfigDescriptorPtr;
	uint16_t                               ConfigDescriptorSize;
	uint8_t                                ConfigAttributes;

	Endpoint_Discard_Word();

	uint16_t wIndex = Endpoint_Read_Word_LE();
	
	switch (bmRequestType & CONTROL_REQTYPE_RECIPIENT)
	{
		case REQREC_DEVICE:
			if (USB_GetDescriptor(((DTYPE_Configuration << 8) | USB_ConfigurationNumber), 0,
			                      (void*)&ConfigDescriptorPtr, &ConfigDescriptorSize) == false)
			{
				return;
			}
			
#if defined(USE_RAM_DESCRIPTORS)
			ConfigAttributes = ConfigDescriptorPtr->ConfigAttributes;
#elif defined(USE_EEPROM_DESCRIPTORS)
			ConfigAttributes = eeprom_read_byte(&ConfigDescriptorPtr->ConfigAttributes);
#else
			ConfigAttributes = pgm_read_byte(&ConfigDescriptorPtr->ConfigAttributes);
#endif

			if (USB_CurrentlySelfPowered)
			  CurrentStatus |= FEATURE_SELFPOWERED_ENABLED;
			
			if (USB_RemoteWakeupEnabled)
			  CurrentStatus |= FEATURE_REMOTE_WAKEUP_ENABLED;
			
			break;
		case REQREC_INTERFACE:
			// No bits set, all bits currently reserved
				
			break;
		case REQREC_ENDPOINT:
			Endpoint_SelectEndpoint(wIndex);

			CurrentStatus = Endpoint_IsStalled();

			Endpoint_SelectEndpoint(ENDPOINT_CONTROLEP);			  
			break;
		default:
			return;
	}
	
	Endpoint_ClearSetupReceived();
	
	Endpoint_Write_Word_LE(CurrentStatus);

	Endpoint_ClearSetupIN();
	
	while (!(Endpoint_IsSetupOUTReceived()));
	Endpoint_ClearSetupOUT();
}

static void USB_Device_ClearSetFeature(const uint8_t bRequest, const uint8_t bmRequestType)
{
	uint16_t wValue = Endpoint_Read_Word_LE();
	uint16_t wIndex = Endpoint_Read_Word_LE();
	
	switch (bmRequestType & CONTROL_REQTYPE_RECIPIENT)
	{
		case REQREC_ENDPOINT:
			if (wValue == FEATURE_ENDPOINT_HALT)
			{
				uint8_t EndpointIndex = (wIndex & ENDPOINT_EPNUM_MASK);
				
				if (EndpointIndex != ENDPOINT_CONTROLEP)
				{
					Endpoint_SelectEndpoint(EndpointIndex);

					if (Endpoint_IsEnabled())
					{				
						if (bRequest == REQ_ClearFeature)
						{
							Endpoint_ClearStall();
							Endpoint_ResetFIFO(EndpointIndex);
							Endpoint_ResetDataToggle();
						}
						else
						{
							Endpoint_StallTransaction();						
						}
					}

					Endpoint_SelectEndpoint(ENDPOINT_CONTROLEP);
					Endpoint_ClearSetupReceived();
					Endpoint_ClearSetupIN();
				}
			}
			
			break;
	}
}

#endif
