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

/** \file
 *
 *  Version constants for informational purposes and version-specific macro creation. This header file contains the
 *  current MyUSB version number in several forms, for use in the user-application (for example, for printing out 
 *  whilst debugging, or for testing for version compatibility).
 */

#ifndef __MYUSB_VERSION_H__
#define __MYUSB_VERSION_H__

	/* Public Interface - May be used in end-application: */
		/* Macros: */
			/** Indicates the major version number of the library as an integer. */
			#define MYUSB_VERSION_MAJOR       1

			/** Indicates the minor version number of the library as an integer. */
			#define MYUSB_VERSION_MINOR       5

			/** Indicates the revision version number of the library as an integer. */
			#define MYUSB_VERSION_REVISION    2

			/** Indicates the complete version number of the library, in string form. */
			#define MYUSB_VERSION_STRING      "1.5.2"

#endif
