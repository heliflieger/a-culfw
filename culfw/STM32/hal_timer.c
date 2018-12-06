/**
  ******************************************************************************
  * File Name          : hal_timer.c
  * Description        : This file provides code for the configuration
  *                      of the TIM instances.
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
#include "hal_timer.h"
#include "board.h"
#include <stm32f1xx_hal.h>
#include <stm32f103xb.h>
#include "rf_mode.h"

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim4;

/* TIM1 init function */
void MX_TIM1_Init(void)
{
  TIM_ClockConfigTypeDef sClockSourceConfig;
  TIM_MasterConfigTypeDef sMasterConfig;

  htim1.Instance = TIM1;
  htim1.Init.Prescaler = (uint32_t)(SystemCoreClock / 10000) - 1;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 80 - 1;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }

  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_TIM_Base_Start_IT(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
}

/* TIM2 init function */
void MX_TIM2_Init(void)
{
  TIM_ClockConfigTypeDef sClockSourceConfig;
  TIM_MasterConfigTypeDef sMasterConfig;

  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 71;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 4000;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }

  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }

  __HAL_TIM_ENABLE(&htim2);

}

/* TIM2 init function */
void MX_TIM3_Init(void)
{
  TIM_ClockConfigTypeDef sClockSourceConfig;
  TIM_MasterConfigTypeDef sMasterConfig;

  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 71;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 4000;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }

  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }

  __HAL_TIM_ENABLE(&htim3);

}

/* TIM2 init function */
void MX_TIM4_Init(void)
{
  TIM_ClockConfigTypeDef sClockSourceConfig;
  TIM_MasterConfigTypeDef sMasterConfig;

  htim4.Instance = TIM4;
  htim4.Init.Prescaler = 71;
  htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim4.Init.Period = 4000;
  htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  if (HAL_TIM_Base_Init(&htim4) != HAL_OK)
  {
    Error_Handler();
  }

  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim4, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim4, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }

  __HAL_TIM_ENABLE(&htim4);

}
void HAL_TIM_Base_MspInit(TIM_HandleTypeDef* tim_baseHandle)
{

  if(tim_baseHandle->Instance==TIM1)
  {
    /* Peripheral clock enable */
    __HAL_RCC_TIM1_CLK_ENABLE();

    /* Peripheral interrupt init */
    HAL_NVIC_SetPriority(TIM1_UP_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(TIM1_UP_IRQn);
  }
  else if(tim_baseHandle->Instance==TIM2)
  {
    /* Peripheral clock enable */
    __HAL_RCC_TIM2_CLK_ENABLE();

    /* Peripheral interrupt init */
    HAL_NVIC_SetPriority(TIM2_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(TIM2_IRQn);
  }
  else if(tim_baseHandle->Instance==TIM3)
  {
    /* Peripheral clock enable */
    __HAL_RCC_TIM3_CLK_ENABLE();

    /* Peripheral interrupt init */
    HAL_NVIC_SetPriority(TIM3_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(TIM3_IRQn);
  }
  else if(tim_baseHandle->Instance==TIM4)
  {
    /* Peripheral clock enable */
    __HAL_RCC_TIM4_CLK_ENABLE();

    /* Peripheral interrupt init */
    HAL_NVIC_SetPriority(TIM4_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(TIM4_IRQn);
  }
}

void HAL_TIM_Base_MspDeInit(TIM_HandleTypeDef* tim_baseHandle)
{

  if(tim_baseHandle->Instance==TIM1)
  {
    __HAL_RCC_TIM1_CLK_DISABLE();

    /* Peripheral interrupt Deinit*/
    HAL_NVIC_DisableIRQ(TIM1_UP_IRQn);
  }
  else if(tim_baseHandle->Instance==TIM2)
  {
    /* Peripheral clock disable */
    __HAL_RCC_TIM2_CLK_DISABLE();

    /* Peripheral interrupt Deinit*/
    HAL_NVIC_DisableIRQ(TIM2_IRQn);
  }
  else if(tim_baseHandle->Instance==TIM3)
  {
    /* Peripheral clock disable */
    __HAL_RCC_TIM3_CLK_DISABLE();

    /* Peripheral interrupt Deinit*/
    HAL_NVIC_DisableIRQ(TIM3_IRQn);
  }
  else if(tim_baseHandle->Instance==TIM4)
  {
    /* Peripheral clock disable */
    __HAL_RCC_TIM4_CLK_DISABLE();

    /* Peripheral interrupt Deinit*/
    HAL_NVIC_DisableIRQ(TIM4_IRQn);
  }
} 

void hal_enable_CC_timer_int(uint8_t instance, uint8_t enable) {
  TIM_HandleTypeDef* htim;

  if(instance == 0)
    htim=&htim2;
  else if(instance == 1)
    htim=&htim3;
  else if(instance == 2)
    htim=&htim4;
  else
    return;

  if (enable) {
    __HAL_TIM_CLEAR_IT(htim, TIM_IT_UPDATE);
    if (HAL_TIM_Base_Start_IT(htim) != HAL_OK) {
      Error_Handler();
    }
  } else {
    __HAL_TIM_DISABLE_IT(htim, TIM_IT_UPDATE);
  }
}
/* USER CODE BEGIN 1 */

__weak void clock_TimerElapsedCallback(void)
{
  /* NOTE : This function Should not be modified, when the callback is needed,
            the HAL_TIMEx_CommutationCallback could be implemented in the user file
   */
}

__weak void rf_receive_TimerElapsedCallback(void)
{
  /* NOTE : This function Should not be modified, when the callback is needed,
            the HAL_TIMEx_CommutationCallback could be implemented in the user file
   */
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
#ifdef HAS_MULTI_CC
  uint8_t old_instance = CC1101.instance;

  if(htim->Instance==TIM1) {
    clock_TimerElapsedCallback();
  } else if(htim->Instance==TIM2) {
    CC1101.instance = 0;
    rf_receive_TimerElapsedCallback();
    CC1101.instance = old_instance;
  } else if(htim->Instance==TIM3) {
    CC1101.instance = 1;
    rf_receive_TimerElapsedCallback();
    CC1101.instance = old_instance;
  } else if(htim->Instance==TIM4) {
    CC1101.instance = 2;
    rf_receive_TimerElapsedCallback();
    CC1101.instance = old_instance;
  }
#else
  if(htim->Instance==TIM1) {
    clock_TimerElapsedCallback();
  } else if(htim->Instance==TIM2) {
    rf_receive_TimerElapsedCallback();
  }
#endif

}

uint32_t HAL_timer_get_reload_register(uint8_t instance) {
  if(instance == 0)
    return TIM2->ARR;
  else if(instance == 1)
    return TIM3->ARR;
  else if(instance == 2)
    return TIM4->ARR;
  else
    return 0;
}

void HAL_timer_set_reload_register(uint8_t instance, uint32_t value) {
  if(instance == 0)
    TIM2->ARR = value;
  else if(instance == 1)
    TIM3->ARR = value;
  else if(instance == 2)
    TIM4->ARR = value;
  else
    return;
}

uint32_t HAL_timer_get_counter_value(uint8_t instance) {
  if(instance == 0)
    return TIM2->CNT;
  else if(instance == 1)
    return TIM3->CNT;
  else if(instance == 2)
    return TIM4->CNT;
  else
    return 0;
}

void HAL_timer_set_counter_value(uint8_t instance, uint32_t value) {
  if(instance == 0)
    TIM2->CNT = value;
  else if(instance == 1)
    TIM3->CNT = value;
  else if(instance == 2)
    TIM4->CNT = value;
  else
    return;
}

void HAL_timer_reset_counter_value(uint8_t instance) {
  if(instance == 0)
    TIM2->CNT = 0;
  else if(instance == 1)
    TIM3->CNT = 0;
  else if(instance == 2)
    TIM4->CNT = 0;
  else
    return;
}



/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
