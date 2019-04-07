/**
  ******************************************************************************
  * File Name          : gpio.c
  * Description        : This file provides code for the configuration
  *                      of all used GPIO pins.
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
#include "hal_gpio.h"
#include <board.h>
#include <string.h>
#include "stm32f103xb.h"
#include "led.h"
#include "delay.h"
#include "rf_mode.h"

transceiver_t CCtransceiver[] = CCTRANSCEIVERS;
uint8_t i2cPort;

/*----------------------------------------------------------------------------*/
/* Configure GPIO                                                             */
/*----------------------------------------------------------------------------*/

void MX_GPIO_Init(void)
{
  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
}

void hal_UCBD_connect_init(void)
{
#ifdef USBD_CONNECT_PIN
  GPIO_InitTypeDef GPIO_InitStruct;

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(USBD_CONNECT_PORT, _BV(USBD_CONNECT_PIN), GPIO_PIN_RESET);

  /*Configure GPIO pins*/
  GPIO_InitStruct.Pin = _BV(USBD_CONNECT_PIN);
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(USBD_CONNECT_PORT, &GPIO_InitStruct);
#endif
}

/*----------------------------------------------------------------------------*/
/* LED                                                                        */
/*----------------------------------------------------------------------------*/

void HAL_LED_Init(void) {
  GPIO_InitTypeDef GPIO_InitStruct;

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LED_GPIO, _BV(LED_PIN), GPIO_PIN_RESET);

  /*Configure GPIO pins : PB1 PB9 */
  GPIO_InitStruct.Pin = _BV(LED_PIN);
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(LED_GPIO, &GPIO_InitStruct);

  #ifdef LED2_GPIO
  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LED2_GPIO, _BV(LED2_PIN), GPIO_PIN_RESET);

  /*Configure GPIO pins : PB1 PB9 */
  GPIO_InitStruct.Pin = _BV(LED2_PIN);
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(LED2_GPIO, &GPIO_InitStruct);

  #endif
}

void HAL_LED_Set(uint8_t led, uint8_t state) {
  switch(led) {
    case LED0:
      HAL_GPIO_WritePin(LED_GPIO, _BV(LED_PIN), state);
      break;
  #ifdef LED2_GPIO
    case LED1:
      HAL_GPIO_WritePin(LED2_GPIO, _BV(LED2_PIN), state);
        break;
  #endif
  #ifdef LED3_GPIO
    case LED2:
      HAL_GPIO_WritePin(LED3_GPIO, _BV(LED3_PIN), state);
        break;
  #endif
  }
}

void HAL_LED_Toggle(uint8_t led) {
  switch(led) {
  case LED0:
    HAL_GPIO_TogglePin(LED_GPIO, _BV(LED_PIN));
    break;
#ifdef LED2_GPIO
  case LED1:
      HAL_GPIO_TogglePin(LED2_GPIO, _BV(LED2_PIN));
      break;
#endif
#ifdef LED3_GPIO
  case LED2:
      HAL_GPIO_TogglePin(LED3_GPIO, _BV(LED3_PIN));
      break;
#endif
  }
}

/*----------------------------------------------------------------------------*/
/* CC1101                                                                     */
/*----------------------------------------------------------------------------*/

void hal_CC_GDO_init(uint8_t cc_num, uint8_t mode) {
  GPIO_InitTypeDef GPIO_InitStruct;

  if(!(cc_num < CCCOUNT ))
    return;

  /*Configure GDO0 (out) pin */
  if(CCtransceiver[cc_num].base[CC_Pin_Out]) {
    if(mode == INIT_MODE_IN_CS_IN) {
      GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
      GPIO_InitStruct.Pull = GPIO_PULLUP;
    } else {
      HAL_GPIO_WritePin(CCtransceiver[cc_num].base[CC_Pin_Out], _BV(CCtransceiver[cc_num].pin[CC_Pin_Out]), GPIO_PIN_RESET);
      GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
      GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    }
    GPIO_InitStruct.Pin = _BV(CCtransceiver[cc_num].pin[CC_Pin_Out]);
    HAL_GPIO_Init(CCtransceiver[cc_num].base[CC_Pin_Out], &GPIO_InitStruct);
  }

  /*Configure GDO1 (CS) pin */
  if(CCtransceiver[cc_num].base[CC_Pin_CS]) {
    HAL_GPIO_WritePin(CCtransceiver[cc_num].base[CC_Pin_CS], _BV(CCtransceiver[cc_num].pin[CC_Pin_CS]), GPIO_PIN_SET);
    GPIO_InitStruct.Pin = _BV(CCtransceiver[cc_num].pin[CC_Pin_CS]);
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(CCtransceiver[cc_num].base[CC_Pin_CS], &GPIO_InitStruct);
  }

  /*Configure GDO2 (in) pin */
  if(CCtransceiver[cc_num].base[CC_Pin_In]) {
    GPIO_InitStruct.Pin = _BV(CCtransceiver[cc_num].pin[CC_Pin_In]);
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(CCtransceiver[cc_num].base[CC_Pin_In], &GPIO_InitStruct);
  }

  HAL_NVIC_SetPriority(EXTI0_IRQn, 2, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);

  HAL_NVIC_SetPriority(EXTI4_IRQn, 2, 0);
  HAL_NVIC_EnableIRQ(EXTI4_IRQn);

  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 2, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 2, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

}


void hal_enable_CC_GDOin_int(uint8_t cc_num, uint8_t enable) {
  GPIO_InitTypeDef GPIO_InitStruct;

  if(!(cc_num < CCCOUNT ))
    return;

  /*Configure in pin */
  GPIO_InitStruct.Pin = _BV(CCtransceiver[cc_num].pin[CC_Pin_In]);
  if (enable)
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  else
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_DeInit(CCtransceiver[cc_num].base[CC_Pin_In], _BV(CCtransceiver[cc_num].pin[CC_Pin_In]));
  HAL_GPIO_Init(CCtransceiver[cc_num].base[CC_Pin_In], &GPIO_InitStruct);
}

void hal_CC_Pin_Set(uint8_t cc_num, CC_PIN pin, GPIO_PinState state) {
  if((cc_num < CCCOUNT) && CCtransceiver[cc_num].base[pin]){
    HAL_GPIO_WritePin(CCtransceiver[cc_num].base[pin], _BV(CCtransceiver[cc_num].pin[pin]), state);
  }
}

uint32_t hal_CC_Pin_Get(uint8_t cc_num, CC_PIN pin) {
  if((cc_num < CCCOUNT) && CCtransceiver[cc_num].base[pin]) {
    return (CCtransceiver[cc_num].base[pin]->IDR) & _BV(CCtransceiver[cc_num].pin[pin]);
  }
  return 0;
}

void hal_CC_move_transceiver_pins(uint8_t source, uint8_t dest) {
  transceiver_t tempCC;
  memcpy(&tempCC, &CCtransceiver[dest], sizeof(transceiver_t));
  memcpy(&CCtransceiver[dest], &CCtransceiver[source], sizeof(transceiver_t));
  memcpy(&CCtransceiver[source], & tempCC, sizeof(transceiver_t));
}

void hal_I2C_Set_Port(uint8_t port) {
  i2cPort= port;
}

__weak void CC1100_in_callback()
{
  /* NOTE : This function Should not be modified, when the callback is needed,
            the HAL_GPIO_EXTI_Callback could be implemented in the user file
   */
}

__weak void CC1100_in_callback1()
{
  /* NOTE : This function Should not be modified, when the callback is needed,
            the HAL_GPIO_EXTI_Callback could be implemented in the user file
   */
}

/**
  * @brief EXTI line detection callbacks
  * @retval None
  */
void hal_GPIO_EXTI_IRQHandler(void) {
#ifdef HAS_MULTI_CC
  uint8_t old_instance = CC1101.instance;

  if(__HAL_GPIO_EXTI_GET_IT(_BV(CCtransceiver[0].pin[CC_Pin_In])) != RESET) {
    __HAL_GPIO_EXTI_CLEAR_IT(_BV(CCtransceiver[0].pin[CC_Pin_In]));
    CC1101.instance = 0;
    CC1100_in_callback();
    CC1101.instance = old_instance;
  }

  if(__HAL_GPIO_EXTI_GET_IT(_BV(CCtransceiver[1].pin[CC_Pin_In])) != RESET) {
    __HAL_GPIO_EXTI_CLEAR_IT(_BV(CCtransceiver[1].pin[CC_Pin_In]));
    CC1101.instance = 1;
    CC1100_in_callback();
    CC1101.instance = old_instance;
  }

  if(__HAL_GPIO_EXTI_GET_IT(_BV(CCtransceiver[2].pin[CC_Pin_In])) != RESET) {
    __HAL_GPIO_EXTI_CLEAR_IT(_BV(CCtransceiver[2].pin[CC_Pin_In]));
    CC1101.instance = 2;
    CC1100_in_callback();
    CC1101.instance = old_instance;
  }

#else
  if(__HAL_GPIO_EXTI_GET_IT(_BV(CCtransceiver[0].pin[CC_Pin_In])) != RESET) {
    __HAL_GPIO_EXTI_CLEAR_IT(_BV(CCtransceiver[0].pin[CC_Pin_In]));
    CC1100_in_callback();
  }
#endif
}

/*----------------------------------------------------------------------------*/
/* Wiznet                                                                     */
/*----------------------------------------------------------------------------*/
#ifdef HAS_WIZNET
void hal_wiznet_Init(void) {
  GPIO_InitTypeDef GPIO_InitStruct;

  /*Configure CS pin Output Level */
  HAL_GPIO_WritePin( WIZNET_CS_GPIO, _BV(WIZNET_CS_PIN), GPIO_PIN_SET);

  /*Configure CS pin */
  GPIO_InitStruct.Pin = _BV(WIZNET_CS_PIN);
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init( WIZNET_CS_GPIO, &GPIO_InitStruct);

  /*Configure RESET pin Output Level */
  HAL_GPIO_WritePin(WIZNET_RST_GPIO, _BV(WIZNET_RST_PIN), GPIO_PIN_SET);

  /*Configure RESET pin */
  GPIO_InitStruct.Pin = _BV(WIZNET_RST_PIN);
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(WIZNET_RST_GPIO, &GPIO_InitStruct);

  // Reset chip
  HAL_GPIO_WritePin(WIZNET_RST_GPIO, _BV(WIZNET_RST_PIN), GPIO_PIN_RESET);
  my_delay_ms(10);
  HAL_GPIO_WritePin(WIZNET_RST_GPIO, _BV(WIZNET_RST_PIN), GPIO_PIN_SET);
  my_delay_ms(10);

}
#endif

/* USER CODE END 2 */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
