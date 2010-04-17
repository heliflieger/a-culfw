#include "led.h"
#include "ringbuffer.h"
#include "cdc.h"

/* Globals: */
CDC_Line_Coding_t LineCoding = { BaudRateBPS: 9600,
                                 CharFormat:  OneStopBit,
                                 ParityType:  Parity_None,
                                 DataBits:    8            };

HANDLES_EVENT(USB_ConfigurationChanged)
{
  Endpoint_ConfigureEndpoint(CDC_NOTIFICATION_EPNUM, EP_TYPE_INTERRUPT,
      ENDPOINT_DIR_IN, CDC_NOTIFICATION_EPSIZE, ENDPOINT_BANK_SINGLE);

  Endpoint_ConfigureEndpoint(CDC_TX_EPNUM, EP_TYPE_BULK,
      ENDPOINT_DIR_IN, USB_BUFSIZE, ENDPOINT_BANK_DOUBLE);

  Endpoint_ConfigureEndpoint(CDC_RX_EPNUM, EP_TYPE_BULK,
      ENDPOINT_DIR_OUT, USB_BUFSIZE, ENDPOINT_BANK_DOUBLE);
}

//////////////////////////////////////////////
// Implement the "Modem" Part of the ACM Spec., i.e ignore the requests.
HANDLES_EVENT(USB_UnhandledControlPacket)
{
  // Process CDC specific control requests
  switch (bRequest)
  {
    case GET_LINE_CODING:
      if(bmRequestType == (REQDIR_DEVICETOHOST|REQTYPE_CLASS|REQREC_INTERFACE)){
        Endpoint_ClearSetupReceived();

        Endpoint_Write_Control_Stream_LE(&LineCoding, sizeof(LineCoding));
 
        Endpoint_ClearSetupOUT();
      }
 
      break;
    case SET_LINE_CODING:
      if(bmRequestType == (REQDIR_HOSTTODEVICE|REQTYPE_CLASS|REQREC_INTERFACE)){
        Endpoint_ClearSetupReceived();

        Endpoint_Read_Control_Stream_LE(&LineCoding, sizeof(LineCoding));

        Endpoint_ClearSetupIN();
      }
 
      break;
    case SET_CONTROL_LINE_STATE:
      if(bmRequestType == (REQDIR_HOSTTODEVICE|REQTYPE_CLASS|REQREC_INTERFACE)){
 
        Endpoint_ClearSetupIN();
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

  if(!inCDC_TASK && Endpoint_ReadWriteAllowed()){ // USB -> RingBuffer

    while (Endpoint_BytesInEndpoint()) {          // Discard data on buffer full
      rb_put(&TTY_Rx_Buffer, Endpoint_Read_Byte());
    }
    Endpoint_ClearCurrentBank(); 
    inCDC_TASK = 1;
    output_flush_func = CDC_Task;
    input_handle_func(DISPLAY_USB);
    inCDC_TASK = 0;
  }


  Endpoint_SelectEndpoint(CDC_TX_EPNUM);          // Then data out
  if(TTY_Tx_Buffer.nbytes && Endpoint_ReadWriteAllowed()) {

    cli();
    while(TTY_Tx_Buffer.nbytes &&
          (Endpoint_BytesInEndpoint() < USB_BUFSIZE))
      Endpoint_Write_Byte(rb_get(&TTY_Tx_Buffer));
    sei();
    
    Endpoint_ClearCurrentBank();                  // Send the data

  }
}

void
cdc_flush()
{
  Endpoint_SelectEndpoint(CDC_TX_EPNUM);
  Endpoint_ClearCurrentBank();
}
