#ifndef _CDC_H_
#define _CDC_H_

#include <avr/interrupt.h>
/* Includes: */
#include <avr/io.h>

#include "ringbuffer.h"

#ifdef SAM7
#include <usb/device/cdc-serial/CDCDSerialDriver.h>
#include <usb/device/cdc-serial/CDCDSerialDriverDescriptors.h>

#include "ttydata.h"

/// Size in bytes of the buffer used for reading data from the USB
#define DATABUFFERSIZEOUT \
    BOARD_USB_ENDPOINTS_MAXPACKETSIZE(CDCDSerialDriverDescriptors_DATAOUT)

#elif defined STM32
#include <usb_device.h>
#include <usbd_cdc_if.h>

#include "ttydata.h"

#define USBD_STATUS_SUCCESS   USBD_OK
#define DATABUFFERSIZEOUT     128

#else
#include <Drivers/USB/USB.h>     // USB Functionality

#include "Descriptors.h"
#endif

/* Macros: */
#define GET_LINE_CODING			0x21
#define SET_LINE_CODING			0x20
#define SET_CONTROL_LINE_STATE		0x22

/* Event Handlers: */
#ifdef LUFA
  #define USB_IsConnected (USB_DeviceState == DEVICE_STATE_Configured)
#endif

/* Type Defines: */
typedef struct
{
        uint32_t BaudRateBPS;
        uint8_t  CharFormat;
        uint8_t  ParityType;
        uint8_t  DataBits;
} CDC_Line_Coding_t;

/* Enums: */
enum CDC_Line_Coding_Format_t
{
        OneStopBit          = 0,
        OneAndAHalfStopBits = 1,
        TwoStopBits         = 2,
};

enum CDC_Line_Codeing_Parity_t
{
        Parity_None         = 0,
        Parity_Odd          = 1,
        Parity_Even         = 2,
        Parity_Mark         = 3,
        Parity_Space        = 4,
};

extern uint8_t cdctask_enabled;

void CDC_Task(void);
void cdc_flush(void);

#endif
