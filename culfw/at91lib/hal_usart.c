/**
  ******************************************************************************
  * File Name          : USART.c
  * Description        : This file provides code for the configuration
  *                      of the USART instances.
  ******************************************************************************
  *
  * COPYRIGHT(c) 2016 STMicroelectronics
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "board.h"
#include "hal_usart.h"

#include <aic/aic.h>
#include <pio/pio.h>
#include <pmc/pmc.h>
#include <usart/usart.h>


static const Pin pins[] = {
    PIN_USART0_RXD,
    PIN_USART0_TXD
};

//static uint8_t inbyte1;
//static uint8_t inbyte2;
//static uint8_t inbyte3;



__attribute__((weak)) void UART_Tx_Callback(void)
{
  /* NOTE : This function Should not be modified, when the callback is needed,
            the CDCDSerialDriver_Receive_Callback could be implemented in the user file
   */
}

__attribute__((weak)) void UART_Rx_Callback(uint8_t data)
{
  /* NOTE : This function Should not be modified, when the callback is needed,
            the CDCDSerialDriver_Receive_Callback could be implemented in the user file
   */
}

void ISR_Usart0(void)
{
    unsigned int status;

    // Read USART status
    status = AT91C_BASE_US0->US_CSR;

    // Receive buffer is full
    if ((status & AT91C_US_RXRDY) == AT91C_US_RXRDY) {

      uint8_t data = AT91C_BASE_US0->US_RHR;
      UART_Rx_Callback(data);

    }
    // Transmitt buffer is empty
    if ((status & AT91C_US_TXRDY) == AT91C_US_TXRDY) {

      UART_Tx_Callback();
      status = AT91C_BASE_US0->US_CSR;
      if ((status & AT91C_US_TXEMPTY) == AT91C_US_TXEMPTY) {
        AT91C_BASE_US0->US_IDR = AT91C_US_TXRDY;
      }
    }

}

void USART1_UART_Init(unsigned int baudrate) {
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
}

void HAL_UART_Set_Baudrate(uint8_t UART_num, uint32_t baudrate) {

  switch(UART_num) {
  case UART_NUM:
    USART1_UART_Init(baudrate);
    break;
  default:
    return;
  }

}

uint32_t HAL_UART_Get_Baudrate(uint8_t UART_num) {

  //switch(UART_num) {
  //case 0:
  //  return huart2.Init.BaudRate;
  //}
  return 0;
}

void HAL_UART_Write(uint8_t UART_num, uint8_t* Buf, uint16_t Len) {
  switch(UART_num) {
  case UART_NUM:
    AT91C_BASE_US0->US_THR = *Buf;
    AT91C_BASE_US0->US_IER = AT91C_US_TXRDY;
    break;
  }
  return;
}

void HAL_UART_init(uint8_t UART_num) {
  switch(UART_num) {
  case UART_NUM:
    USART1_UART_Init(115200);
    break;
  }
  return;
}

uint8_t HAL_UART_TX_is_idle(uint8_t UART_num) {

  switch(UART_num) {
  case UART_NUM:
    if ((AT91C_BASE_US0->US_CSR & AT91C_US_TXEMPTY) == AT91C_US_TXEMPTY) {
      return 1;
    }
    break;
  default:
    return 0;
  }


  return 0;
}


