#include <Drivers/USB/HighLevel/StdRequestType.h>
#include <Drivers/USB/HighLevel/USBTask.h>  // for USB_ControlRequest
#include <Drivers/USB/LowLevel/Endpoint.h>
#include <Drivers/USB/LowLevel/LowLevel.h>
#include <avr/interrupt.h>              // for cli, sei
#include <stdbool.h>                    // for bool
#include <stdint.h>                     // for uint8_t

#include "Descriptors.h"                // for CDC_TX_EPNUM, CDC_RX_EPNUM, etc
#include "board.h"                      // for USB_BUFSIZE
#include "cdc.h"                        // for CDC_Line_Coding_t, etc
#include "display.h"                    // for DISPLAY_USB
#include "ttydata.h"                    // IWYU pragma: keep (for TTY_Rx_Buffer)

#undef CDC_FUNCTIONAL_DESCRIPTOR
#include <Drivers/USB/Class/Common/CDC.h>


/* Globals: */
CDC_Line_Coding_t LineCoding = { BaudRateBPS: 9600,
                                 CharFormat:  OneStopBit,
                                 ParityType:  Parity_None,
                                 DataBits:    8            };


void EVENT_USB_Device_ConfigurationChanged(void)
{
  Endpoint_ConfigureEndpoint(CDC_NOTIFICATION_EPNUM, EP_TYPE_INTERRUPT,
      ENDPOINT_DIR_IN, CDC_NOTIFICATION_EPSIZE, ENDPOINT_BANK_SINGLE);

  Endpoint_ConfigureEndpoint(CDC_TX_EPNUM, EP_TYPE_BULK,
      ENDPOINT_DIR_IN, USB_BUFSIZE, ENDPOINT_BANK_SINGLE);

  Endpoint_ConfigureEndpoint(CDC_RX_EPNUM, EP_TYPE_BULK,
      ENDPOINT_DIR_OUT, USB_BUFSIZE, ENDPOINT_BANK_SINGLE);

  LineCoding.BaudRateBPS = 0;
}

//////////////////////////////////////////////
// Implement the "Modem" Part of the ACM Spec., i.e ignore the requests.
void EVENT_USB_Device_UnhandledControlRequest(void)
{
  uint8_t bmRequestType = USB_ControlRequest.bmRequestType;

  switch (USB_ControlRequest.bRequest) {
    case REQ_GetLineEncoding:
      if(bmRequestType == (REQDIR_DEVICETOHOST|REQTYPE_CLASS|REQREC_INTERFACE)){
        Endpoint_ClearSETUP();
        Endpoint_Write_Control_Stream_LE(&LineCoding,sizeof(LineCoding));
        Endpoint_ClearOUT();
      }
      break;

    case REQ_SetLineEncoding:
      if(bmRequestType ==
                 (REQDIR_HOSTTODEVICE | REQTYPE_CLASS | REQREC_INTERFACE)) {
        Endpoint_ClearSETUP();
        Endpoint_Read_Control_Stream_LE(&LineCoding, sizeof(LineCoding));
        Endpoint_ClearIN();
      }
      break;

    case REQ_SetControlLineState:
      if(bmRequestType ==
                 (REQDIR_HOSTTODEVICE | REQTYPE_CLASS | REQREC_INTERFACE)) {
        Endpoint_ClearSETUP();
        Endpoint_ClearStatusStage();
      }
      break;

  }

}


////////////////////
// Fill data from USB to the RingBuffer and vice-versa
void
CDC_Task(void)
{
  static char inCDC_TASK = 0;

  if(!USB_IsConnected)
    return;

  Endpoint_SelectEndpoint(CDC_RX_EPNUM);          // First data in
  if(!inCDC_TASK && Endpoint_IsReadWriteAllowed()){ // USB -> RingBuffer

    while (Endpoint_BytesInEndpoint()) {          // Discard data on buffer full
      rb_put(&TTY_Rx_Buffer, Endpoint_Read_Byte());
    }
    Endpoint_ClearOUT(); 

    inCDC_TASK = 1;
    output_flush_func = CDC_Task;
    input_handle_func(DISPLAY_USB);
    inCDC_TASK = 0;
  }


  Endpoint_SelectEndpoint(CDC_TX_EPNUM);          // Then data out
  if(TTY_Tx_Buffer.nbytes && Endpoint_IsReadWriteAllowed()) {

    cli();
    while(TTY_Tx_Buffer.nbytes &&
          (Endpoint_BytesInEndpoint() < USB_BUFSIZE))
      Endpoint_Write_Byte(rb_get(&TTY_Tx_Buffer));
    sei();
    
    bool IsFull = (Endpoint_BytesInEndpoint() == USB_BUFSIZE);
    Endpoint_ClearIN();                  // Send the data
    if(IsFull) {
      Endpoint_WaitUntilReady();
      Endpoint_ClearIN();
    }
  }
}

void
cdc_flush()
{
  Endpoint_SelectEndpoint(CDC_TX_EPNUM);
  Endpoint_WaitUntilReady();
  Endpoint_ClearIN();
}
