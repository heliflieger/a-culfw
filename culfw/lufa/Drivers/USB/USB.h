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

/** \file
 *
 *  Master include file for the library USB functionality. This file should be included in all user projects making
 *  use of the USB portions of the library, instead of including any headers in the USB/LowLevel or USB/HighLevel
 *  directories.
 */

/** @defgroup Group_USB USB - LUFA/Drivers/USB/USB.h
 *
 *  \section Sec_Dependencies Module Source Dependencies
 *  The following files must be built with any user project that uses this module:
 *    - LUFA/Drivers/USB/LowLevel/DevChapter9.c
 *    - LUFA/Drivers/USB/LowLevel/Endpoint.c
 *    - LUFA/Drivers/USB/LowLevel/Host.c
 *    - LUFA/Drivers/USB/LowLevel/HostChapter9.c
 *    - LUFA/Drivers/USB/LowLevel/LowLevel.c
 *    - LUFA/Drivers/USB/LowLevel/Pipe.c
 *    - LUFA/Drivers/USB/HighLevel/Events.c
 *    - LUFA/Drivers/USB/HighLevel/USBInterrupt.c
 *    - LUFA/Drivers/USB/HighLevel/USBTask.c
 *    - LUFA/Drivers/USB/HighLevel/ConfigDescriptor.c
 *
 *  \section Module Description
 *  Driver and framework for the USB controller hardware on the USB series of AVR microcontrollers. This module
 *  consists of many submodules, and is designed to provide an easy way to configure and control USB host, device
 *  or OTG mode USB applications.
 */

/** \ingroup Group_USB
 *  @defgroup Group_USBClassDrivers USB Class Drivers
 *
 *  Drivers for both host and device mode of the standard USB classes, for rapid application development.
 *  Class drivers give a framework which sits on top of the low level library API, allowing for standard
 *  USB classes to be implemented in a project with minimal user code. These drivers can be used in
 *  conjunction with the library low level APIs to implement interfaces both via the class drivers and via
 *  the standard library APIs.
 *
 *  Multiple device mode class drivers can be used within a project, including multiple instances of the
 *  same class driver. In this way, USB Hosts and Devices can be made quickly using the internal class drivers
 *  so that more time and effort can be put into the end application instead of the USB protocol.
 *
 *  \warning The Host mode Class Drivers are currently incomplete and are included for preview purposes only. The Host
 *           mode class drivers should not be used in any user project until they have been completed in a later revision.
 */
 
#ifndef __USB_H__
#define __USB_H__

	/* Preprocessor Checks: */		
		#if (!(defined(__AVR_AT90USB1287__) || defined(__AVR_AT90USB647__) ||  \
		       defined(__AVR_AT90USB1286__) || defined(__AVR_AT90USB646__) ||  \
			   defined(__AVR_AT90USB162__)  || defined(__AVR_AT90USB82__)  ||  \
			   defined(__AVR_ATmega32U2__)  || defined(__AVR_ATmega16U2__) ||  \
			   defined(__AVR_ATmega8U2__)   ||                                 \
			   defined(__AVR_ATmega16U4__)  || defined(__AVR_ATmega32U4__) ||  \
			   defined(__AVR_ATmega32U6__)))
			#error The currently selected AVR model is not supported under the USB component of the LUFA library.
		#endif
		
	/* Includes: */
        // IWYU pragma: begin_exports
		#include "HighLevel/USBMode.h"
		#include "HighLevel/USBTask.h"
		#include "HighLevel/USBInterrupt.h"
		#include "HighLevel/Events.h"
		#include "HighLevel/StdDescriptors.h"

		#include "LowLevel/LowLevel.h"
	
		#if defined(USB_CAN_BE_HOST) || defined(__DOXYGEN__)
			#include "LowLevel/Host.h"
			#include "LowLevel/HostChapter9.h"
			#include "LowLevel/Pipe.h"
		#endif
		
		#if defined(USB_CAN_BE_DEVICE) || defined(__DOXYGEN__)
			#include "LowLevel/Device.h"
			#include "LowLevel/DevChapter9.h"
			#include "LowLevel/Endpoint.h"
		#endif
		
		#if defined(USB_CAN_BE_BOTH) || defined(__DOXYGEN__)
			#include "LowLevel/OTG.h"
		#endif
		
		#include "HighLevel/ConfigDescriptor.h"
	// IWYU pragma: end_exports
		
#endif

