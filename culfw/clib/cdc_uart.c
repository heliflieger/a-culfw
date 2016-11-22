/*
 * cdc_uart.c
 *
 *  Created on: 01.12.2016
 *      Author: Telekatz
 */

#include "ringbuffer.h"
#include "cdc.h"
#include "usbd_cdc_if.h"
#include "hal_usart.h"
#include "board.h"

#ifdef CDC_COUNT

#if CDC_COUNT > 1
uint8_t CDC1_Tx_buffer[CDC_DATA_HS_MAX_PACKET_SIZE];
uint8_t CDC1_Tx_len = 0;
rb_t UART2_Tx_Buffer;


uint8_t CDCDSerialDriver_Receive_Callback1(uint8_t* Buf, uint32_t *Len) {
  HAL_UART_Transmit_IT(&huart2, Buf, (uint16_t)*Len);
  return 0;
}

void UART2_Rx_Callback(uint8_t data) {
  rb_put(&UART2_Tx_Buffer, data);
}
#endif

#if CDC_COUNT > 2
uint8_t CDC2_Tx_buffer[CDC_DATA_HS_MAX_PACKET_SIZE];
uint8_t CDC2_Tx_len = 0;
rb_t UART3_Tx_Buffer;

uint8_t CDCDSerialDriver_Receive_Callback2(uint8_t* Buf, uint32_t *Len) {
  HAL_UART_Transmit_IT(&huart3, Buf, (uint16_t)*Len);
  return 0;
}

void UART3_Rx_Callback(uint8_t data) {
  rb_put(&UART3_Tx_Buffer, data);
}
#endif


void cdc_uart_init(void) {
  MX_USART2_UART_Init();
  MX_USART3_UART_Init();
}


void cdc_uart_task(void) {

#if CDC_COUNT > 1
  if(CDC_isConnected(CDC1)) {
    if(UART2_Tx_Buffer.nbytes) {
      while(UART2_Tx_Buffer.nbytes && CDC1_Tx_len<CDC_DATA_HS_MAX_PACKET_SIZE) {
        CDC1_Tx_buffer[CDC1_Tx_len++] = rb_get(&UART2_Tx_Buffer);
      }
    }

    if(CDC1_Tx_len) {
      uint8_t ret = CDCDSerialDriver_Write(CDC1_Tx_buffer,CDC1_Tx_len, 0, CDC1);
      if( ret == USBD_STATUS_SUCCESS) {
        CDC1_Tx_len=0;
      }
    }
  }
#endif

#if CDC_COUNT > 2
  if(CDC_isConnected(CDC2)) {
    if(UART3_Tx_Buffer.nbytes) {
      while(UART3_Tx_Buffer.nbytes && CDC2_Tx_len<CDC_DATA_HS_MAX_PACKET_SIZE) {
        CDC2_Tx_buffer[CDC2_Tx_len++] = rb_get(&UART3_Tx_Buffer);
      }
    }

    if(CDC2_Tx_len) {
      uint8_t ret = CDCDSerialDriver_Write(CDC2_Tx_buffer,CDC2_Tx_len, 0, CDC2);
      if( ret == USBD_STATUS_SUCCESS) {
        CDC2_Tx_len=0;
      }
    }
  }
#endif

}

#endif

