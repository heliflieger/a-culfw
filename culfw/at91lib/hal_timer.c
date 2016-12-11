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

void hal_enable_CC_timer_int(uint8_t enable) {
  if (enable) {
    AT91C_BASE_TC1->TC_SR;
    #ifdef LONG_PULSE
        AT91C_BASE_TC1->TC_CMR &= ~(AT91C_TC_CPCTRG);
    #endif
    AT91C_BASE_AIC->AIC_IECR= 1 << AT91C_ID_TC1;
  } else {
    AT91C_BASE_AIC->AIC_IDCR = 1 << AT91C_ID_TC1; //Disable Interrupt
    #ifdef LONG_PULSE
    AT91C_BASE_TC1->TC_CMR |= AT91C_TC_CPCTRG;
    #endif
  }
}
/* USER CODE BEGIN 1 */

__attribute__((weak)) void clock_TimerElapsedCallback(void)
{
  /* NOTE : This function Should not be modified, when the callback is needed,
            the HAL_TIMEx_CommutationCallback could be implemented in the user file
   */
}

__attribute__((weak)) void rf_receive_TimerElapsedCallback(void)
{
  /* NOTE : This function Should not be modified, when the callback is needed,
            the HAL_TIMEx_CommutationCallback could be implemented in the user file
   */
}

void ISR_Timer0() {
  // Clear status bit to acknowledge interrupt
  AT91C_BASE_TC0->TC_SR;
  clock_TimerElapsedCallback();
}

void ISR_Timer1() {
  // Clear status bit to acknowledge interrupt
  AT91C_BASE_TC1->TC_SR;
  rf_receive_TimerElapsedCallback();
}



/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
