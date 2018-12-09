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
#include "hal_gpio.h"
#include "usbd_cdc_if.h"
#include "usb_device.h"
#include "utility/dbgu.h"
#ifdef HAS_WIZNET
#include "ethernet.h"
#endif
#include "atomic.h"

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;
UART_HandleTypeDef huart3;

static uint8_t inbyte1;
static uint8_t inbyte2;
static uint8_t inbyte3;

/* USART1 init function */

void MX_USART1_UART_Init(void)
{

  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  HAL_UART_Receive_IT(&huart1, &inbyte1, 1);
}
/* USART2 init function */

void MX_USART2_UART_Init(void)
{

  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  HAL_UART_Receive_IT(&huart2, &inbyte2, 1);
}
/* USART3 init function */

void MX_USART3_UART_Init(void)
{

  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  HAL_UART_Receive_IT(&huart3, &inbyte3, 1);
}

void HAL_UART_MspInit(UART_HandleTypeDef* uartHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct;
  if(uartHandle->Instance==USART1)
  {
    /* Peripheral clock enable */
    __HAL_RCC_USART1_CLK_ENABLE();
  
    /**USART1 GPIO Configuration    
    PA9     ------> USART1_TX
    PA10     ------> USART1_RX 
    */
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* Peripheral interrupt init */
    HAL_NVIC_SetPriority(USART1_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
  /* USER CODE BEGIN USART1_MspInit 1 */

  }
  else if(uartHandle->Instance==USART2)
  {
    /* Peripheral clock enable */
    __HAL_RCC_USART2_CLK_ENABLE();
  
    /**USART2 GPIO Configuration    
    PA2     ------> USART2_TX
    PA3     ------> USART2_RX 
    */
    GPIO_InitStruct.Pin = GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* Peripheral interrupt init */
    HAL_NVIC_SetPriority(USART2_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(USART2_IRQn);
  }
  else if(uartHandle->Instance==USART3)
  {
    /* Peripheral clock enable */
    __HAL_RCC_USART3_CLK_ENABLE();
  
    /**USART3 GPIO Configuration    
    PB10     ------> USART3_TX
    PB11     ------> USART3_RX 
    */
    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_11;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* Peripheral interrupt init */
    HAL_NVIC_SetPriority(USART3_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(USART3_IRQn);
  }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef* uartHandle)
{

  if(uartHandle->Instance==USART1)
  {
    __HAL_RCC_USART1_CLK_DISABLE();
  
    /**USART1 GPIO Configuration    
    PA9     ------> USART1_TX
    PA10     ------> USART1_RX 
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_9|GPIO_PIN_10);
  }
  else if(uartHandle->Instance==USART2)
  {
    __HAL_RCC_USART2_CLK_DISABLE();
  
    /**USART2 GPIO Configuration    
    PA2     ------> USART2_TX
    PA3     ------> USART2_RX 
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_2|GPIO_PIN_3);
  }
  else if(uartHandle->Instance==USART3)
  {
    __HAL_RCC_USART3_CLK_DISABLE();
  
    /**USART3 GPIO Configuration    
    PB10     ------> USART3_TX
    PB11     ------> USART3_RX 
    */
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_10|GPIO_PIN_11);
  }
} 

__weak void UART_Tx_Callback(void)
{
  /* NOTE : This function Should not be modified, when the callback is needed,
            the CDCDSerialDriver_Receive_Callback could be implemented in the user file
   */
}

__weak void UART_Rx_Callback(uint8_t data)
{
  /* NOTE : This function Should not be modified, when the callback is needed,
            the CDCDSerialDriver_Receive_Callback could be implemented in the user file
   */
}

__weak void UART2_Rx_Callback(uint8_t data)
{
  /* NOTE : This function Should not be modified, when the callback is needed,
            the CDCDSerialDriver_Receive_Callback could be implemented in the user file
   */
}

__weak void UART3_Rx_Callback(uint8_t data)
{
  /* NOTE : This function Should not be modified, when the callback is needed,
            the CDCDSerialDriver_Receive_Callback could be implemented in the user file
   */
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *UartHandle)
{

  if(UartHandle->Instance==USART1) {
#ifdef HAS_UART
    UART_Rx_Callback(inbyte1);
#else
    DBGU_RxByte = inbyte1;
    DBGU_RxReady = 1;
#endif
    ATOMIC_BLOCK() {
      HAL_UART_Receive_IT(&huart1, &inbyte1, 1);
    }

  } else if(UartHandle->Instance==USART2) {
    UART2_Rx_Callback(inbyte2);
    ATOMIC_BLOCK() {
      HAL_UART_Receive_IT(&huart2, &inbyte2, 1);
    }

  } else if(UartHandle->Instance==USART3) {
    UART3_Rx_Callback(inbyte3);
    ATOMIC_BLOCK() {
      HAL_UART_Receive_IT(&huart3, &inbyte3, 1);
    }

  }

  /* Set transmission flag: transfer complete */

  return;
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *UartHandle)
{
  if(UartHandle->Instance==USART1) {
#ifdef HAS_UART
    UART_Tx_Callback();
#endif
  } else if(UartHandle->Instance==USART2) {
    CDC_Receive_next(CDC1);
#ifdef HAS_WIZNET
    NET_Receive_next(NET1);
#endif

  } else if(UartHandle->Instance==USART3) {
    CDC_Receive_next(CDC2);
#ifdef HAS_WIZNET
    NET_Receive_next(NET2);
#endif

  }

  return;
}

void HAL_UART_Set_Baudrate(uint8_t UART_num, uint32_t baudrate) {
  UART_HandleTypeDef *UartHandle;


  switch(UART_num) {
  case 0:
    UartHandle = &huart2;
    break;
  case 1:
    UartHandle = &huart3;
    break;
  case UART_NUM:
    UartHandle = &huart1;
    break;
  default:
    return;
  }

  UartHandle->Init.BaudRate = baudrate;
  UART_SetConfig(UartHandle);
}

uint32_t HAL_UART_Get_Baudrate(uint8_t UART_num) {

  switch(UART_num) {
  case 0:
    return huart2.Init.BaudRate;
  case 1:
    return huart3.Init.BaudRate;
  }
  return 0;
}

void HAL_UART_Write(uint8_t UART_num, uint8_t* Buf, uint16_t Len) {
  ATOMIC_BLOCK() {
    switch(UART_num) {
    case 0:
      HAL_UART_Transmit_IT(&huart2, Buf, Len);
      break;
    case 1:
      HAL_UART_Transmit_IT(&huart3, Buf, Len);
      break;
    case UART_NUM:
      HAL_UART_Transmit_IT(&huart1, Buf, Len);
      break;
    }
  }
  return;
}

void HAL_UART_init(uint8_t UART_num) {
  switch(UART_num) {
  case 0:
    MX_USART2_UART_Init();
    break;
  case 1:
    MX_USART3_UART_Init();
    break;
  case UART_NUM:
    MX_USART1_UART_Init();
    break;
  }
  return;
}

uint8_t HAL_UART_TX_is_idle(uint8_t UART_num) {
  UART_HandleTypeDef *UartHandle;


  switch(UART_num) {
  case 0:
    UartHandle = &huart2;
    break;
  case 1:
    UartHandle = &huart3;
    break;
  case UART_NUM:
    UartHandle = &huart1;
    break;
  default:
    return 0;
  }

   if((UartHandle->State == HAL_UART_STATE_READY) || (UartHandle->State == HAL_UART_STATE_BUSY_RX)) {
     return 1;
   }

  return 0;
}

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
