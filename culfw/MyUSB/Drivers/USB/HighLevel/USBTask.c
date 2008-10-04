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

#include "../LowLevel/USBMode.h"

#define INCLUDE_FROM_USBTASK_C
#include "USBTask.h"

volatile bool      USB_IsConnected;
volatile bool      USB_IsInitialized;

#if defined(USB_CAN_BE_BOTH)
         TaskPtr_t USB_TaskPtr;
#endif

#if defined(USB_CAN_BE_HOST)
volatile uint8_t   USB_HostState;
#endif

TASK(USB_USBTask)
{
	#if defined(USB_HOST_ONLY)
		USB_HostTask();
	#elif defined(USB_DEVICE_ONLY)
		USB_DeviceTask();
	#else
		if (USB_IsInitialized)
		  (*USB_TaskPtr)();
	#endif
}

#if defined(USB_CAN_BE_BOTH)
void USB_InitTaskPointer(void)
{
	if (USB_CurrentMode != USB_MODE_NONE)
	{
		if (USB_CurrentMode == USB_MODE_DEVICE)
		  USB_TaskPtr = (TaskPtr_t)USB_DeviceTask;
		else
		  USB_TaskPtr = (TaskPtr_t)USB_HostTask;

		USB_IsInitialized = true;
	}
}
#endif

#if defined(USB_CAN_BE_DEVICE)
static void USB_DeviceTask(void)
{
	if (USB_IsConnected)
	{
		uint8_t PrevEndpoint = Endpoint_GetCurrentEndpoint();
	
		Endpoint_SelectEndpoint(ENDPOINT_CONTROLEP);

		if (Endpoint_IsSetupReceived())
		  USB_Device_ProcessControlPacket();
		  
		Endpoint_SelectEndpoint(PrevEndpoint);
	}
}
#endif

#if defined(USB_CAN_BE_HOST)
static void USB_HostTask(void)
{
	uint8_t ErrorCode = HOST_ENUMERROR_NoError;

	switch (USB_HostState)
	{
		case HOST_STATE_Attached:
			if (USB_INT_HasOccurred(USB_INT_DCONNI))
			{	
				USB_INT_Clear(USB_INT_DCONNI);
				USB_INT_Clear(USB_INT_DDISCI);

				USB_IsConnected = true;
				RAISE_EVENT(USB_Connect);
					
				USB_Host_ResumeBus();
				Pipe_ClearPipes();
				
				if (USB_Host_WaitMS(100) != HOST_WAITERROR_Successful)
				{
					ErrorCode = HOST_ENUMERROR_WaitStage;
					break;
				}
					
				USB_Host_ResetDevice();
					
				if (USB_Host_WaitMS(100) != HOST_WAITERROR_Successful)
				{
					ErrorCode = HOST_ENUMERROR_WaitStage;
					break;
				}

				USB_HostState = HOST_STATE_Powered;
			}
			
			if (USB_INT_HasOccurred(USB_INT_BCERRI))
			{
				USB_INT_Clear(USB_INT_BCERRI);

				ErrorCode = HOST_ENUMERROR_NoDeviceDetected;
				break;
			}
				
			break;
		case HOST_STATE_Powered:
			if (USB_Host_WaitMS(100) != HOST_WAITERROR_Successful)
			{
				ErrorCode = HOST_ENUMERROR_WaitStage;
				break;
			}
 
			Pipe_ConfigurePipe(PIPE_CONTROLPIPE, EP_TYPE_CONTROL,
							   PIPE_TOKEN_SETUP, PIPE_CONTROLPIPE,
							   PIPE_CONTROLPIPE_DEFAULT_SIZE, PIPE_BANK_SINGLE);

			USB_HostState = HOST_STATE_Default;
			
			break;
		case HOST_STATE_Default:
			USB_HostRequest = (USB_Host_Request_Header_t)
				{
					bmRequestType: (REQDIR_DEVICETOHOST | REQTYPE_STANDARD | REQREC_DEVICE),
					bRequest:      REQ_GetDescriptor,
					wValue:        (DTYPE_Device << 8),
					wIndex:        0,
					wLength:       PIPE_CONTROLPIPE_DEFAULT_SIZE,
				};

			#if defined(USE_NONSTANDARD_DESCRIPTOR_NAMES)
			uint8_t* DataBuffer = alloca(offsetof(USB_Descriptor_Device_t, Endpoint0Size) + 1);
			#else
			uint8_t* DataBuffer = alloca(offsetof(USB_Descriptor_Device_t, bMaxPacketSize0) + 1);			
			#endif
			
			if (USB_Host_SendControlRequest(DataBuffer) != HOST_SENDCONTROL_Successful)
			{
				ErrorCode = HOST_ENUMERROR_ControlError;
				break;
			}
			
			#if defined(USE_NONSTANDARD_DESCRIPTOR_NAMES)
			USB_ControlPipeSize = DataBuffer[offsetof(USB_Descriptor_Device_t, Endpoint0Size)];
			#else
			USB_ControlPipeSize = DataBuffer[offsetof(USB_Descriptor_Device_t, bMaxPacketSize0)];			
			#endif
			
			USB_Host_ResetDevice();
			
			if (USB_Host_WaitMS(200) != HOST_WAITERROR_Successful)
			{
				ErrorCode = HOST_ENUMERROR_WaitStage;
				break;
			}

			Pipe_DisablePipe();
			Pipe_DeallocateMemory();
			Pipe_ResetPipe(PIPE_CONTROLPIPE);
			
			Pipe_ConfigurePipe(PIPE_CONTROLPIPE, EP_TYPE_CONTROL,
			                   PIPE_TOKEN_SETUP, PIPE_CONTROLPIPE,
			                   USB_ControlPipeSize, PIPE_BANK_SINGLE);

			if (!(Pipe_IsConfigured()))
			{
				ErrorCode = HOST_ENUMERROR_PipeConfigError;
				break;
			}

			Pipe_SetInfiniteINRequests();

			USB_HostRequest = (USB_Host_Request_Header_t)
				{
					bmRequestType: (REQDIR_HOSTTODEVICE | REQTYPE_STANDARD | REQREC_DEVICE),
					bRequest:      REQ_SetAddress,
					wValue:        USB_HOST_DEVICEADDRESS,
					wIndex:        0,
					wLength:       0,
				};

			if (USB_Host_SendControlRequest(NULL) != HOST_SENDCONTROL_Successful)
			{
				ErrorCode = HOST_ENUMERROR_ControlError;
				break;
			}

			USB_Host_SetDeviceAddress(USB_HOST_DEVICEADDRESS);

			USB_Host_WaitMS(10);
			
			RAISE_EVENT(USB_DeviceEnumerationComplete);
			USB_HostState = HOST_STATE_Addressed;

			break;
	}

	if (ErrorCode != HOST_ENUMERROR_NoError)
	{
		RAISE_EVENT(USB_DeviceEnumerationFailed, ErrorCode);

		USB_Host_VBUS_Auto_Off();

		RAISE_EVENT(USB_DeviceUnattached);
		
		if (USB_IsConnected)
		  RAISE_EVENT(USB_Disconnect);

		USB_Host_PrepareForDeviceConnect();
	}
}
#endif
