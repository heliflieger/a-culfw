
#include <avr/interrupt.h>
#include <avr/io.h>

#include "display.h"
#include "ringbuffer.h"
#include "ttydata.h"
#ifdef USE_HAL
#include "hal_usart.h"
#include "cdc.h"
#endif

//#include "led.h"
#include "serial.h"

void (*usbinfunc)(void);

#ifdef USE_HAL

void UART_Tx_Callback(void) {
  static uint8_t TXdata;

  if(!USB_IsConnected) {
    if (TTY_Tx_Buffer.nbytes) {
      TXdata = rb_get(&TTY_Tx_Buffer);
      HAL_UART_Write(UART_NUM, &TXdata, 1);
    }
  }
}

void UART_Rx_Callback(uint8_t data) {
  if(!USB_IsConnected) {
    rb_put(&TTY_Rx_Buffer, data);
  }
}

#else
// TX complete (data buffer empty) 
ISR(USART_UDRE_vect)
{
     if (TTY_Tx_Buffer.nbytes) {
	  
	  UDR0 = rb_get(&TTY_Tx_Buffer);
	  
     } else {
	  
	  UCSR0B &= ~_BV(UDRIE0);
	  
     }
     
}

// RX complete
ISR(USART_RX_vect)
{

     //LED_TOGGLE();

     /* read UART status register and UART data register */ 
     uint8_t data = UDR0;
     uint8_t usr  = UCSR0A;
     
     if ((usr & (_BV(FE0)|_BV(DOR0))) == 0)
	  rb_put(&TTY_Rx_Buffer, data);
     
}
#endif

void uart_init(unsigned int baudrate) 
{
#ifdef USE_HAL
    HAL_UART_init(UART_NUM);
    HAL_UART_Set_Baudrate(UART_NUM,baudrate);

#else
     /* Set baud rate */
     if ( baudrate & 0x8000 ) 
     {
	  UCSR0A = (1<<U2X0);  //Enable 2x speed 
	  baudrate &= ~0x8000;
     }

     UBRR0 = baudrate;
     
     /* Enable USART receiver and transmitter and receive complete interrupt */
     UCSR0B = _BV(RXCIE0)|(1<<RXEN0)|(1<<TXEN0);
    
     /* Set frame format: asynchronous, 8data, no parity, 1stop bit */   
     UCSR0C = (1<<UCSZ00)|(1<<UCSZ01);

#endif
}

void uart_task(void) 
{
     input_handle_func(DISPLAY_USB);
     uart_flush();
}

void uart_flush(void) 
{
#ifdef USE_HAL
    if (TTY_Tx_Buffer.nbytes && HAL_UART_TX_is_idle(UART_NUM) ) {
      UART_Tx_Callback();
    }
#else
     if (!bit_is_set(UCSR0B, UDRIE0) && TTY_Tx_Buffer.nbytes)
	  UCSR0B |= _BV(UDRIE0);
#endif
     
}
