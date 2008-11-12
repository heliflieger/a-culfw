//////////////////////////////////////////////
//
// Serial modem emulation
//
//////////////////////////////////////////////
// Connecting to USB we receive
// - USB_ConfigurationChanged,
// - USB_UnhandledControlPacket:22:21, 20:21
// DisConnecting from USB we receive nothing
// Close/Open 
// - USB_UnhandledControlPacket:22:21
//
// 20: is SET_LINE_CODING, 22: is SET_CONTROL_LINE_STATE
// :21 is REQDIR_HOSTTODEVICE | REQTYPE_CLASS | REQREC_INTERFACE
// We try to follow this with usb_device_open

#include "led.h"
#include "ringbuffer.h"
#include "cdc.h"

#if 0                   // debugging
#include "board.h"
#include "pcf8833.h"
#include "display.h"
#endif

void (*usbinfunc)(void);

/* Globals: */
CDC_Line_Coding_t LineCoding = { BaudRateBPS: 9600,
                                 CharFormat:  OneStopBit,
                                 ParityType:  Parity_None,
                                 DataBits:    8            };

DEFINE_RBUF(USB_Tx_Buffer, CDC_TX_EPSIZE)
DEFINE_RBUF(USB_Rx_Buffer, CDC_RX_EPSIZE)

EVENT_HANDLER(USB_ConfigurationChanged)
{
  // Setup CDC Notification, Rx and Tx Endpoints
  Endpoint_ConfigureEndpoint(CDC_NOTIFICATION_EPNUM, EP_TYPE_INTERRUPT,
      ENDPOINT_DIR_IN, CDC_NOTIFICATION_EPSIZE, ENDPOINT_BANK_SINGLE);

  Endpoint_ConfigureEndpoint(CDC_TX_EPNUM, EP_TYPE_BULK,
      ENDPOINT_DIR_IN, CDC_RX_EPSIZE, ENDPOINT_BANK_DOUBLE);

  Endpoint_ConfigureEndpoint(CDC_RX_EPNUM, EP_TYPE_BULK,
      ENDPOINT_DIR_OUT, CDC_TX_EPSIZE, ENDPOINT_BANK_DOUBLE);

  Scheduler_SetTaskMode(CDC_Task, TASK_RUN);	
}

//////////////////////////////////////////////
// Implement the "Modem" Part of the ACM Spec., i.e ignore the requests.
EVENT_HANDLER(USB_UnhandledControlPacket)
{
  uint8_t* LineCodingData = (uint8_t*)&LineCoding;

  // Process CDC specific control requests
  switch (bRequest)
  {
    case GET_LINE_CODING:
      if(bmRequestType == (REQDIR_DEVICETOHOST|REQTYPE_CLASS|REQREC_INTERFACE)){
        // Acknowedge the SETUP packet, ready for data transfer
        Endpoint_ClearSetupReceived();

        // Write the line coding data to the control endpoint
        Endpoint_Write_Control_Stream_LE(LineCodingData, sizeof(LineCoding));
 
        // Send the line coding data to the host and clear the control endpoint
        Endpoint_ClearSetupOUT();
      }
 
      break;
    case SET_LINE_CODING:
      if(bmRequestType == (REQDIR_HOSTTODEVICE|REQTYPE_CLASS|REQREC_INTERFACE)){
        // Acknowedge the SETUP packet, ready for data transfer
        Endpoint_ClearSetupReceived();

        // Read the line coding data in from the host into the global struct
        Endpoint_Read_Control_Stream_LE(LineCodingData, sizeof(LineCoding));

        // Send the line coding data to the host and clear the control endpoint
        Endpoint_ClearSetupIN();
      }
 
      break;
    case SET_CONTROL_LINE_STATE:
      if(bmRequestType == (REQDIR_HOSTTODEVICE|REQTYPE_CLASS|REQREC_INTERFACE)){
        // Acknowedge the SETUP packet, ready for data transfer
        Endpoint_ClearSetupReceived();
 
        // Send an empty packet to acknowedge the command (currently unused)
        Endpoint_ClearSetupIN();
      }

      break;
  }
}


////////////////////
// Fill data from USB to the RingBuffer and vice-versa
TASK(CDC_Task)
{
  static char inCDC_TASK = 0;

  if(!USB_IsConnected)
    return;

  Endpoint_SelectEndpoint(CDC_RX_EPNUM);          // First data in

  if(!inCDC_TASK && Endpoint_ReadWriteAllowed()){ // USB -> RingBuffer

    while (Endpoint_BytesInEndpoint()) {          // Discard data on buffer full
      rb_put(USB_Rx_Buffer, Endpoint_Read_Byte());
    }
    Endpoint_ClearCurrentBank(); 
    inCDC_TASK = 1;
    usbinfunc();
    inCDC_TASK = 0;
  }


  Endpoint_SelectEndpoint(CDC_TX_EPNUM);          // Then data out
  if(USB_Tx_Buffer->nbytes && Endpoint_ReadWriteAllowed()) {

    cli();
    while(USB_Tx_Buffer->nbytes &&
          (Endpoint_BytesInEndpoint() < CDC_TX_EPSIZE))
      Endpoint_Write_Byte(rb_get(USB_Tx_Buffer));
    sei();
    
    Endpoint_ClearCurrentBank();                  // Send the data

  }
}
