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
#include <hal_gpio.h>
#include <board.h>
/* USER CODE BEGIN 0 */
#include "stm32f103xb.h"
#include "led.h"
/* USER CODE END 0 */

/*----------------------------------------------------------------------------*/
/* Configure GPIO                                                             */
/*----------------------------------------------------------------------------*/
/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/** Configure pins as 
        * Analog 
        * Input 
        * Output
        * EVENT_OUT
        * EXTI
*/
void MX_GPIO_Init(void)
{

  GPIO_InitTypeDef GPIO_InitStruct;

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_RESET);

  /*Configure GPIO pins : PB1 PB9 */
  GPIO_InitStruct.Pin = GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

}

/* USER CODE BEGIN 2 */
void HAL_LED_Init(void) {
  GPIO_InitTypeDef GPIO_InitStruct;

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LED_GPIO, _BV(LED_PIN), GPIO_PIN_RESET);

  /*Configure GPIO pins : PB1 PB9 */
  GPIO_InitStruct.Pin = _BV(LED_PIN);
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(LED_GPIO, &GPIO_InitStruct);

}

void hal_CC_GDO_init(void) {
  GPIO_InitTypeDef GPIO_InitStruct;

  /*Configure CS pin */
  HAL_GPIO_WritePin(CC1100_CS_GPIO, _BV(CC1100_CS_PIN), GPIO_PIN_SET);
  GPIO_InitStruct.Pin = _BV(CC1100_CS_PIN);
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(CC1100_CS_GPIO, &GPIO_InitStruct);

  /*Configure out pin */
  HAL_GPIO_WritePin(CC1100_OUT_GPIO, _BV(CC1100_OUT_PIN), GPIO_PIN_RESET);
  GPIO_InitStruct.Pin = _BV(CC1100_OUT_PIN);
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(CC1100_OUT_GPIO, &GPIO_InitStruct);

  /*Configure in pin */
  GPIO_InitStruct.Pin = _BV(CC1100_IN_PIN);
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(CC1100_IN_GPIO, &GPIO_InitStruct);

  HAL_NVIC_SetPriority(EXTI0_IRQn, 2, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);

}

//hal_enable_CC_GDOin_int(FALSE);

void hal_enable_CC_GDOin_int(uint8_t enable) {
  GPIO_InitTypeDef GPIO_InitStruct;

  /*Configure in pin */
  GPIO_InitStruct.Pin = _BV(CC1100_IN_PIN);
  if (enable)
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  else
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_DeInit(CC1100_IN_GPIO, _BV(CC1100_IN_PIN));
  HAL_GPIO_Init(CC1100_IN_GPIO, &GPIO_InitStruct);
}

__weak void CC1100_in_callback()
{
  /* NOTE : This function Should not be modified, when the callback is needed,
            the HAL_GPIO_EXTI_Callback could be implemented in the user file
   */
}

/**
  * @brief EXTI line detection callbacks
  * @retval None
  */
void hal_GPIO_EXTI_IRQHandler(void)
{
  if(__HAL_GPIO_EXTI_GET_IT(_BV(CC1100_IN_PIN)) != RESET) {
    __HAL_GPIO_EXTI_CLEAR_IT(_BV(CC1100_IN_PIN));
    CC1100_in_callback();
  }
}
/* USER CODE END 2 */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
