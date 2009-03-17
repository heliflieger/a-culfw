#ifndef _CDC_H_
#define _CDC_H_

/* Includes: */
#include <avr/io.h>
#include <avr/interrupt.h>

#include "Descriptors.h"
#include "ringbuffer.h"

#include <MyUSB/Drivers/USB/USB.h>     // USB Functionality
#include <MyUSB/Scheduler/Scheduler.h> // Simple scheduler for task management

/* Macros: */
#define GET_LINE_CODING			0x21
#define SET_LINE_CODING			0x20
#define SET_CONTROL_LINE_STATE		0x22

/* Event Handlers: */
HANDLES_EVENT(USB_Connect);
HANDLES_EVENT(USB_Disconnect);
HANDLES_EVENT(USB_ConfigurationChanged);
HANDLES_EVENT(USB_UnhandledControlPacket);

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

TASK(CDC_Task);

extern rb_t *const USB_Tx_Buffer;
extern rb_t *const USB_Rx_Buffer;
extern void (*usbinfunc)(void);
void cdc_flush(void);

#endif
