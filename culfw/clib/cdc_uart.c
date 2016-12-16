/*
 * cdc_uart.c
 *
 *  Created on: 01.12.2016
 *      Author: Telekatz
 */

#include "ringbuffer.h"
#include "cdc.h"
#include "usb_device.h"
#include "hal_usart.h"
#include "board.h"
#include <utility/trace.h>
#ifdef HAS_W5100
#include <socket.h>
#include "ethernet.h"
#endif
#include "display.h"
#include "fncollection.h"

#ifdef CDC_COUNT

uint8_t CDC_Tx_buffer[CDC_COUNT-1][CDC_DATA_HS_MAX_PACKET_SIZE];
uint8_t CDC_Tx_len[CDC_COUNT-1];
rb_t UART_Rx_Buffer[CDC_COUNT-1];

uint8_t CDCDSerialDriver_Receive_Callback1(uint8_t* Buf, uint32_t *Len) {
  HAL_UART_Write(0, Buf, (uint16_t)*Len);
  return 0;
}

uint8_t CDCDSerialDriver_Receive_Callback2(uint8_t* Buf, uint32_t *Len) {
  HAL_UART_Write(1, Buf, (uint16_t)*Len);
  return 0;
}

#if CDC_COUNT > 1
void UART2_Rx_Callback(uint8_t data) {
  rb_put(&UART_Rx_Buffer[0], data);
}
#endif

#if CDC_COUNT > 2
void UART3_Rx_Callback(uint8_t data) {
  rb_put(&UART_Rx_Buffer[1], data);
}
#endif

#ifdef HAS_W5100
void EE_write_baud(uint8_t num, uint32_t baud) {
  uint8_t b[4];
  uint32_t* p_baud = (uint32_t*)b;

  *p_baud=baud;

  if(num > 1)
    return ;

  for(uint8_t i = 0; i < 4; i++)
    ewb(EE_CDC1_BAUD+i+(num*4),b[i]);

  return;
}

uint32_t EE_read_baud(uint8_t num) {
  uint8_t b[4];

  if(num > 1)
    return 0;

  for(uint8_t i = 0; i < 4; i++)
    b[i] = erb(EE_CDC1_BAUD+i+(num*4));

  uint32_t* baud = (uint32_t*)b;

  return *baud;
}
#endif

void cdc_uart_init(void) {
  MX_USART2_UART_Init();
  MX_USART3_UART_Init();

#ifdef HAS_W5100
  TRACE_DEBUG("UART Baud: %u@%u\n\r", 0,EE_read_baud(0));
  TRACE_DEBUG("UART Baud: %u@%u\n\r", 1,EE_read_baud(1));

  HAL_UART_Set_Baudrate(0,EE_read_baud(0));
  HAL_UART_Set_Baudrate(1,EE_read_baud(1));
#endif
}


void cdc_uart_task(void) {

#if CDC_COUNT > 1
  for(uint8_t x = 0;x<CDC_COUNT-1;x++) {
    if(UART_Rx_Buffer[x].nbytes) {
      while(UART_Rx_Buffer[x].nbytes && CDC_Tx_len[x] < CDC_DATA_HS_MAX_PACKET_SIZE) {
        CDC_Tx_buffer[x][CDC_Tx_len[x]++] = rb_get(&UART_Rx_Buffer[x]);
      }
    }

    if(CDC_Tx_len[x]) {
      int32_t ret;
      if(CDC_isConnected(CDC1+x)) {
        ret = CDCDSerialDriver_Write(CDC_Tx_buffer[x],CDC_Tx_len[x], 0, CDC1+x);
        if( ret == USBD_STATUS_SUCCESS) {
          #ifdef HAS_W5100
          Net_Write(CDC_Tx_buffer[x], CDC_Tx_len[x], 1+x);

          #endif
          CDC_Tx_len[x]=0;
        }
      } else {
        #ifdef HAS_W5100
        Net_Write(CDC_Tx_buffer[x], CDC_Tx_len[x], 1+x);
        #endif
        CDC_Tx_len[x]=0;
      }
    }
  }
#endif
}

void cdc_uart_func(char *in) {
  if (in[1] == 'i') {                // Info
    uint32_t baud;
    char c;
    for(uint8_t x=0; x<CDC_COUNT-1;x++) {
      DC('0'+x);DC(':');
      baud  = HAL_UART_Get_Baudrate(x);
      c = baud%10 + '0';
      baud /= 10;
      DU(baud ,0);
      DC(c);
      DNL();
    }

  } else if ((in[1] == 'b') && in[3] == '@') {  // set baudrate
    uint32_t baud;
    uint8_t num;
    in[3] = 0;
    fromdec(in+2,&num);
    fromdec32(in+4,&baud);
    TRACE_DEBUG("UART set Baud: %u@%u\n\r", num,baud);
    HAL_UART_Set_Baudrate(num,baud);

#ifdef HAS_W5100
  } else if (in[1] == 's') {  // store baudrate
    EE_write_baud(0,HAL_UART_Get_Baudrate(0));
    EE_write_baud(1,HAL_UART_Get_Baudrate(1));
#endif
  }
}

#endif

