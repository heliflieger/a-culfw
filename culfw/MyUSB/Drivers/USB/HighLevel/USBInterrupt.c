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
#include "USBInterrupt.h"

void USB_INT_DisableAllInterrupts(void)
{
	#if defined(USB_FULL_CONTROLLER)
	USBCON &= ~((1 << VBUSTE) | (1 << IDTE));				
	#elif defined(USB_MODIFIED_FULL_CONTROLLER)
	USBCON &= ~(1 << VBUSTE);					
	#endif
	
	#if defined(USB_CAN_BE_HOST)
	UHIEN   = 0;
	OTGIEN  = 0;
	#endif
	
	#if defined(USB_CAN_BE_DEVICE)
	UDIEN   = 0;
	#endif
}

void USB_INT_ClearAllInterrupts(void)
{
	#if defined(USB_FULL_CONTROLLER) || defined(USB_MODIFIED_FULL_CONTROLLER)
	USBINT  = 0;
	#endif
	
	#if defined(USB_CAN_BE_HOST)
	UHINT   = 0;
	OTGINT  = 0;
	#endif
	
	#if defined(USB_CAN_BE_DEVICE)
	UDINT   = 0;
	#endif
}

ISR(USB_GEN_vect)
{
	#if defined(USB_CAN_BE_DEVICE)
	#if defined(USB_FULL_CONTROLLER) || defined(USB_MODIFIED_FULL_CONTROLLER)
	if (USB_INT_HasOccurred(USB_INT_VBUS) && USB_INT_IsEnabled(USB_INT_VBUS))
	{
		USB_INT_Clear(USB_INT_VBUS);

		RAISE_EVENT(USB_VBUSChange);

		if (USB_VBUS_GetStatus())
		{
			RAISE_EVENT(USB_VBUSConnect);
			
			if (USB_IsInitialized)
			{
				if (USB_IsConnected)
				  RAISE_EVENT(USB_Disconnect);

				USB_SetupInterface();
				
				USB_IsConnected = true;
					
				RAISE_EVENT(USB_Connect);
			}
		}
		else
		{
			RAISE_EVENT(USB_Disconnect);
		
			USB_Detach();
			USB_CLK_Freeze();
			USB_PLL_Off();
			USB_REG_Off();

			RAISE_EVENT(USB_VBUSDisconnect);
			
			USB_INT_Clear(USB_INT_VBUS);
			
			USB_IsConnected = false;
		}
	}
	#endif

	if (USB_INT_HasOccurred(USB_INT_SUSPEND) && USB_INT_IsEnabled(USB_INT_SUSPEND))
	{
		USB_INT_Clear(USB_INT_SUSPEND);

		USB_INT_Disable(USB_INT_SUSPEND);
		USB_INT_Enable(USB_INT_WAKEUP);
		
		USB_CLK_Freeze();
		#if !defined(MANUAL_PLL_CONTROL)
		USB_PLL_Off();
		#endif

		RAISE_EVENT(USB_Suspend);

		#if defined(USB_LIMITED_CONTROLLER)
		USB_IsConnected = false;
		RAISE_EVENT(USB_Disconnect);
		#endif
	}

	if (USB_INT_HasOccurred(USB_INT_WAKEUP) && USB_INT_IsEnabled(USB_INT_WAKEUP))
	{
		#if !defined(MANUAL_PLL_CONTROL)
		USB_PLL_On();
		#endif
		while (!(USB_PLL_IsReady()));

		USB_CLK_Unfreeze();

		USB_INT_Clear(USB_INT_WAKEUP);

		USB_INT_Disable(USB_INT_WAKEUP);
		USB_INT_Enable(USB_INT_SUSPEND);
		
		#if defined(USB_LIMITED_CONTROLLER)
		USB_IsConnected = true;
		RAISE_EVENT(USB_Connect);
		#endif

		RAISE_EVENT(USB_WakeUp);
	}
   
	if (USB_INT_HasOccurred(USB_INT_EORSTI) && USB_INT_IsEnabled(USB_INT_EORSTI))
	{
		USB_INT_Clear(USB_INT_EORSTI);

		USB_ConfigurationNumber = 0;

		USB_INT_Clear(USB_INT_SUSPEND);
		USB_INT_Disable(USB_INT_SUSPEND);
		USB_INT_Enable(USB_INT_WAKEUP);

		Endpoint_ConfigureEndpoint(ENDPOINT_CONTROLEP, EP_TYPE_CONTROL,
		                           ENDPOINT_DIR_OUT, USB_ControlEndpointSize,
		                           ENDPOINT_BANK_SINGLE);

		RAISE_EVENT(USB_Reset);
	}
	#endif
	
	#if defined(USB_CAN_BE_HOST)
	if (USB_INT_HasOccurred(USB_INT_DDISCI) && USB_INT_IsEnabled(USB_INT_DDISCI))
	{
		USB_INT_Clear(USB_INT_DDISCI);
		USB_INT_Clear(USB_INT_DCONNI);
		USB_INT_Disable(USB_INT_DDISCI);
			
		USB_IsConnected = false;
		
		RAISE_EVENT(USB_DeviceUnattached);
		RAISE_EVENT(USB_Disconnect);

		USB_Host_PrepareForDeviceConnect();
	}
	
	if (USB_INT_HasOccurred(USB_INT_VBERRI) && USB_INT_IsEnabled(USB_INT_VBERRI))
	{
		USB_INT_Clear(USB_INT_VBERRI);

		USB_Host_VBUS_Manual_Off();

		USB_IsConnected = false;

		RAISE_EVENT(USB_HostError, HOST_ERROR_VBusVoltageDip);
		RAISE_EVENT(USB_DeviceUnattached);

		USB_HostState   = HOST_STATE_Unattached;
	}

	if (USB_INT_HasOccurred(USB_INT_SRPI) && USB_INT_IsEnabled(USB_INT_SRPI))
	{
		USB_INT_Clear(USB_INT_SRPI);
		USB_INT_Disable(USB_INT_SRPI);
		USB_INT_Disable(USB_INT_BCERRI);
	
		RAISE_EVENT(USB_DeviceAttached);

		USB_Host_VBUS_Manual_Off();

		USB_OTGPAD_On();
		USB_Host_VBUS_Auto_Enable();
		USB_Host_VBUS_Auto_On();

		USB_INT_Enable(USB_INT_DDISCI);
		
		USB_HostState = HOST_STATE_Attached;
	}

	if (USB_INT_HasOccurred(USB_INT_BCERRI) && USB_INT_IsEnabled(USB_INT_BCERRI))
	{
		USB_INT_Clear(USB_INT_BCERRI);
		
		USB_Host_VBUS_Manual_Off();
		USB_Host_PrepareForDeviceConnect();
	}
	#endif

	#if defined(USB_CAN_BE_BOTH)
	if (USB_INT_HasOccurred(USB_INT_IDTI) && USB_INT_IsEnabled(USB_INT_IDTI))
	{		
		USB_INT_Clear(USB_INT_IDTI);

		if (USB_IsConnected)
		{
			if (USB_CurrentMode == USB_MODE_HOST)
			  RAISE_EVENT(USB_DeviceUnattached);
			else
			  RAISE_EVENT(USB_Disconnect);
		}

		RAISE_EVENT(USB_UIDChange);

		USB_Host_VBUS_Manual_Off();
		USB_Host_VBUS_Auto_Enable();
		USB_OTGPAD_On();

		USB_SetupInterface();	
	}
	#endif
}
