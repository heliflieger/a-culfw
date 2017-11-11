/**
  ******************************************************************************
  * File Name          : USART.h
  * Description        : This file provides code for the configuration
  *                      of the USART instances.
  ******************************************************************************
*/

#pragma once

#include <stdint.h>

#define UART_NUM                0xff

extern void Error_Handler(void);

void HAL_UART_Set_Baudrate(uint8_t UART_num, uint32_t baudrate);
uint32_t HAL_UART_Get_Baudrate(uint8_t UART_num);
void HAL_UART_Write(uint8_t UART_num, uint8_t* Buf, uint16_t Len);
void HAL_UART_init(uint8_t UART_num);
uint8_t HAL_UART_TX_is_idle(uint8_t UART_num);

