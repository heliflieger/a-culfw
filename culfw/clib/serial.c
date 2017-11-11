
#include <avr/interrupt.h>
#include <avr/io.h>

#include "display.h"
#include "ringbuffer.h"
#include "ttydata.h"
#ifdef SAM7
#include <aic/aic.h>
#include <pio/pio.h>
#include <pmc/pmc.h>
#include <usart/usart.h>
#include <usb/device/cdc-serial/CDCDSerialDriver.h>

#include "board.h"

static const Pin pins[] = {
    PIN_USART0_RXD,
    PIN_USART0_TXD
};

#endif

#ifdef STM32
#include "hal_usart.h"
#include "cdc.h"
#endif

//#include "led.h"
#include "serial.h"

void (*usbinfunc)(void);

#ifdef SAM7
void ISR_Usart0(void)
{
    unsigned int status;

    // Read USART status
    status = AT91C_BASE_US0->US_CSR;

    // Receive buffer is full
    if ((status & AT91C_US_RXRDY) == AT91C_US_RXRDY) {

      uint8_t data = AT91C_BASE_US0->US_RHR;
      if(!USB_IsConnected)
        rb_put(&TTY_Rx_Buffer, data);

    }
    // Transmitt buffer is empty
    if ((status & AT91C_US_TXRDY) == AT91C_US_TXRDY) {

      if (TTY_Tx_Buffer.nbytes) {
        AT91C_BASE_US0->US_THR = rb_get(&TTY_Tx_Buffer);
      } else {
        AT91C_BASE_US0->US_IDR = AT91C_US_TXRDY;
      }

    }

}
#elif defined STM32

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
#ifdef SAM7
    unsigned int mode = AT91C_US_USMODE_NORMAL
                      | AT91C_US_CLKS_CLOCK
                      | AT91C_US_CHRL_8_BITS
                      | AT91C_US_PAR_NONE
                      | AT91C_US_NBSTOP_1_BIT
                      | AT91C_US_CHMODE_NORMAL;

    // Configure pins
    PIO_Configure(pins, PIO_LISTSIZE(pins));

    // Enable the peripheral clock in the PMC
    PMC_EnablePeripheral(AT91C_ID_US0);

    // Configure the USART in the desired mode
    USART_Configure(AT91C_BASE_US0, mode, baudrate, BOARD_MCK);

    // Configure the RXBUFF interrupt
    AT91C_BASE_US0->US_IER = AT91C_US_RXRDY;
    AIC_ConfigureIT(AT91C_ID_US0, 0, ISR_Usart0);
    AIC_EnableIT(AT91C_ID_US0);

    // Enable receiver & transmitter
    USART_SetTransmitterEnabled(AT91C_BASE_US0, 1);
    USART_SetReceiverEnabled(AT91C_BASE_US0, 1);
#elif defined STM32
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
#ifdef SAM7
  if (!bit_is_set(AT91C_BASE_US0->US_IMR, AT91C_US_TXRDY) && TTY_Tx_Buffer.nbytes)
    AT91C_BASE_US0->US_IER = AT91C_US_TXRDY;
#elif defined STM32
    if (TTY_Tx_Buffer.nbytes && HAL_UART_TX_is_idle(UART_NUM) ) {
      UART_Tx_Callback();
    }

#else
     if (!bit_is_set(UCSR0B, UDRIE0) && TTY_Tx_Buffer.nbytes)
	  UCSR0B |= _BV(UDRIE0);
#endif
     
}
