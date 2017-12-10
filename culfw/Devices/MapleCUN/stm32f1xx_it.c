/**
  ******************************************************************************
  * @file    stm32f1xx_it.c
  * @brief   Interrupt Service Routines.
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
#include "stm32f1xx_hal.h"
#include "stm32f1xx.h"

/* USER CODE BEGIN 0 */
#include "led.h"
#include "hal_gpio.h"
#include "stm32f103xb.h"
#include "hal_usart.h"
#include <utility/trace.h>
#include "board.h"

/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/
extern PCD_HandleTypeDef hpcd_USB_FS;
extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim4;

/******************************************************************************/
/*            Cortex-M3 Processor Interruption and Exception Handlers         */ 
/******************************************************************************/

/**
* @brief This function handles Non maskable interrupt.
*/
void NMI_Handler(void)
{
  /* USER CODE BEGIN NonMaskableInt_IRQn 0 */

  /* USER CODE END NonMaskableInt_IRQn 0 */
  /* USER CODE BEGIN NonMaskableInt_IRQn 1 */

  /* USER CODE END NonMaskableInt_IRQn 1 */
}

/**
* @brief This function handles Hard fault interrupt.
*/
__attribute__((optimize(0))) void HardFault_Handler_1 (unsigned int * hardfault_args)
{
  unsigned int stacked_r0;
  unsigned int stacked_r1;
  unsigned int stacked_r2;
  unsigned int stacked_r3;
  unsigned int stacked_r12;
  unsigned int stacked_lr;
  unsigned int stacked_pc;
  unsigned int stacked_psr;

  stacked_r0 = ((unsigned long) hardfault_args[0]);
  stacked_r1 = ((unsigned long) hardfault_args[1]);
  stacked_r2 = ((unsigned long) hardfault_args[2]);
  stacked_r3 = ((unsigned long) hardfault_args[3]);

  stacked_r12 = ((unsigned long) hardfault_args[4]);
  stacked_lr = ((unsigned long) hardfault_args[5]);
  stacked_pc = ((unsigned long) hardfault_args[6]);
  stacked_psr = ((unsigned long) hardfault_args[7]);

  TRACE_INFO ("\n\n[Hard fault handler - all numbers in hex]\n");
  TRACE_INFO ("R0 = %x\n", stacked_r0);
  TRACE_INFO ("R1 = %x\n", stacked_r1);
  TRACE_INFO ("R2 = %x\n", stacked_r2);
  TRACE_INFO ("R3 = %x\n", stacked_r3);
  TRACE_INFO ("R12 = %x\n", stacked_r12);
  TRACE_INFO ("LR [R14] = %x  subroutine call return address\n", stacked_lr);
  TRACE_INFO ("PC [R15] = %x  program counter\n", stacked_pc);
  TRACE_INFO ("PSR = %x\n", stacked_psr);
  TRACE_INFO ("BFAR = %04x%04x\n",  (uint16_t)((*((volatile unsigned long *)(0xE000ED38))) >> 16),
                                    (uint16_t)((*((volatile unsigned long *)(0xE000ED38))) & 0xffff));
  TRACE_INFO ("CFSR = %04x%04x\n",  (uint16_t)((*((volatile unsigned long *)(0xE000ED28))) >> 16),
                                    (uint16_t)((*((volatile unsigned long *)(0xE000ED28))) & 0xffff));
  TRACE_INFO ("HFSR = %04x%04x\n",  (uint16_t)((*((volatile unsigned long *)(0xE000ED2C))) >> 16),
                                    (uint16_t)((*((volatile unsigned long *)(0xE000ED2C))) & 0xffff));
  TRACE_INFO ("DFSR = %04x%04x\n",  (uint16_t)((*((volatile unsigned long *)(0xE000ED30))) >> 16),
                                    (uint16_t)((*((volatile unsigned long *)(0xE000ED30))) & 0xffff));
  TRACE_INFO ("AFSR = %04x%04x\n",  (uint16_t)((*((volatile unsigned long *)(0xE000ED3C))) >> 16),
                                    (uint16_t)((*((volatile unsigned long *)(0xE000ED3C))) & 0xffff));
  TRACE_INFO ("SCB_SHCSR = %04x%04x\n",   (uint16_t)(SCB->SHCSR >> 16),
                                          (uint16_t)(SCB->SHCSR & 0xffff));

  while (1);
}

__attribute__( ( naked ) ) void HardFault_Handler(void)
{
  /* USER CODE BEGIN HardFault_IRQn 0 */

  __asm volatile
      (
          " tst lr, #4                                                \n"
          " ite eq                                                    \n"
          " mrseq r0, msp                                             \n"
          " mrsne r0, psp                                             \n"
          " ldr r1, [r0, #24]                                         \n"
          " ldr r2, handler2_address_const                            \n"
          " bx r2                                                     \n"
          " handler2_address_const: .word HardFault_Handler_1         \n"
      );
  /* USER CODE END HardFault_IRQn 0 */

  while (1)
  {
  }
  /* USER CODE BEGIN HardFault_IRQn 1 */

  /* USER CODE END HardFault_IRQn 1 */
}

/**
* @brief This function handles Memory management fault.
*/
void MemManage_Handler(void)
{
  /* USER CODE BEGIN MemoryManagement_IRQn 0 */

  /* USER CODE END MemoryManagement_IRQn 0 */
  while (1)
  {
  }
  /* USER CODE BEGIN MemoryManagement_IRQn 1 */

  /* USER CODE END MemoryManagement_IRQn 1 */
}

/**
* @brief This function handles Prefetch fault, memory access fault.
*/
void BusFault_Handler(void)
{
  /* USER CODE BEGIN BusFault_IRQn 0 */

  /* USER CODE END BusFault_IRQn 0 */
  while (1)
  {
  }
  /* USER CODE BEGIN BusFault_IRQn 1 */

  /* USER CODE END BusFault_IRQn 1 */
}

/**
* @brief This function handles Undefined instruction or illegal state.
*/
void UsageFault_Handler(void)
{
  /* USER CODE BEGIN UsageFault_IRQn 0 */

  /* USER CODE END UsageFault_IRQn 0 */
  while (1)
  {
  }
  /* USER CODE BEGIN UsageFault_IRQn 1 */

  /* USER CODE END UsageFault_IRQn 1 */
}

/**
* @brief This function handles System service call via SWI instruction.
*/
void SVC_Handler(void)
{
  /* USER CODE BEGIN SVCall_IRQn 0 */

  /* USER CODE END SVCall_IRQn 0 */
  /* USER CODE BEGIN SVCall_IRQn 1 */

  /* USER CODE END SVCall_IRQn 1 */
}

/**
* @brief This function handles Debug monitor.
*/
void DebugMon_Handler(void)
{
  /* USER CODE BEGIN DebugMonitor_IRQn 0 */

  /* USER CODE END DebugMonitor_IRQn 0 */
  /* USER CODE BEGIN DebugMonitor_IRQn 1 */

  /* USER CODE END DebugMonitor_IRQn 1 */
}

/**
* @brief This function handles Pendable request for system service.
*/
void PendSV_Handler(void)
{
  /* USER CODE BEGIN PendSV_IRQn 0 */

  /* USER CODE END PendSV_IRQn 0 */
  /* USER CODE BEGIN PendSV_IRQn 1 */

  /* USER CODE END PendSV_IRQn 1 */
}

/**
* @brief This function handles System tick timer.
*/
void SysTick_Handler(void)
{
  /* USER CODE BEGIN SysTick_IRQn 0 */

  /* USER CODE END SysTick_IRQn 0 */
  HAL_IncTick();
  HAL_SYSTICK_IRQHandler();
  /* USER CODE BEGIN SysTick_IRQn 1 */

  /* USER CODE END SysTick_IRQn 1 */
}

/******************************************************************************/
/* STM32F1xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32f1xx.s).                    */
/******************************************************************************/

/**
* @brief This function handles USB low priority or CAN RX0 interrupts.
*/
void USB_LP_CAN1_RX0_IRQHandler(void)
{
  HAL_PCD_IRQHandler(&hpcd_USB_FS);
}

/**
* @brief This function handles TIM1 update interrupt.
*/
void TIM1_UP_IRQHandler(void)
{
  HAL_TIM_IRQHandler(&htim1);
}

void TIM2_IRQHandler(void)
{
  HAL_TIM_IRQHandler(&htim2);
}

void TIM3_IRQHandler(void)
{
  HAL_TIM_IRQHandler(&htim3);
}

void TIM4_IRQHandler(void)
{
  HAL_TIM_IRQHandler(&htim4);
}
/**
* @brief This function handles EXTI line0 interrupt.
*/
void EXTI0_IRQHandler(void)
{
  hal_GPIO_EXTI_IRQHandler();
}

/**
* @brief This function handles EXTI line0 interrupt.
*/
void EXTI4_IRQHandler(void)
{
  hal_GPIO_EXTI_IRQHandler();
}

void EXTI9_5_IRQHandler(void)
{
  hal_GPIO_EXTI_IRQHandler();
}

void EXTI15_10_IRQHandler(void)
{
  hal_GPIO_EXTI_IRQHandler();
}

/**
* @brief This function handles USART1 global interrupt.
*/
void USART1_IRQHandler(void)
{
  HAL_UART_IRQHandler(&huart1);
}

/**
* @brief This function handles USART2 global interrupt.
*/
void USART2_IRQHandler(void)
{
  HAL_UART_IRQHandler(&huart2);
}

/**
* @brief This function handles USART3 global interrupt.
*/
void USART3_IRQHandler(void)
{
  HAL_UART_IRQHandler(&huart3);
}
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
