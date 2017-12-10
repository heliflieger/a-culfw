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
#include "rf_mode.h"
#include <aic/aic.h>

void hal_enable_CC_timer_int(uint8_t instance, uint8_t enable) {

  AT91PS_TC  timer;
  uint32_t   timerID;

  if(instance == 0) {
    timer=AT91C_BASE_TC1;
    timerID=AT91C_ID_TC1;
  } else if(instance == 1) {
    timer=AT91C_BASE_TC2;
    timerID=AT91C_ID_TC2;
  } else
    return;

  if (enable) {
    timer->TC_SR;
    #ifdef LONG_PULSE
    timer->TC_CMR &= ~(AT91C_TC_CPCTRG);
    #endif
    AT91C_BASE_AIC->AIC_IECR= 1 << timerID;
  } else {
    AT91C_BASE_AIC->AIC_IDCR = 1 << timerID; //Disable Interrupt
    #ifdef LONG_PULSE
    timer->TC_CMR |= AT91C_TC_CPCTRG;
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
#ifdef USE_RF_MODE
  uint8_t old_instance = CC1101.instance;
  CC1101.instance = 0;
  rf_receive_TimerElapsedCallback();
  CC1101.instance = old_instance;
#else
  rf_receive_TimerElapsedCallback();
#endif
}

void ISR_Timer2() {
  // Clear status bit to acknowledge interrupt
  AT91C_BASE_TC2->TC_SR;
#ifdef USE_RF_MODE
  uint8_t old_instance = CC1101.instance;
  CC1101.instance = 1;
  rf_receive_TimerElapsedCallback();
  CC1101.instance = old_instance;
#else
  rf_receive_TimerElapsedCallback();
#endif
}

void HAL_timer_set_reload_register(uint8_t instance, uint32_t value) {
  if(instance == 0)
    (AT91C_BASE_TC1->TC_RC) = value;
  else if(instance == 1)
    (AT91C_BASE_TC2->TC_RC) = value;
  else
    return;
}

uint32_t HAL_timer_get_counter_value(uint8_t instance) {
  if(instance == 0)
    return AT91C_BASE_TC1->TC_CV;
  else if(instance == 1)
    return AT91C_BASE_TC2->TC_CV;
  else
    return 0;
}

void HAL_timer_reset_counter_value(uint8_t instance) {
  if(instance == 0)
    AT91C_BASE_TC1->TC_CCR = AT91C_TC_CLKEN | AT91C_TC_SWTRG;
  else if(instance == 1)
    AT91C_BASE_TC2->TC_CCR = AT91C_TC_CLKEN | AT91C_TC_SWTRG;
  else
    return;
}

void HAL_timer_init() {
  AT91C_BASE_PMC->PMC_PCER = (1 << AT91C_ID_TC0);
  AT91C_BASE_TC0->TC_CCR = AT91C_TC_CLKDIS;
  AT91C_BASE_TC0->TC_IDR = 0xFFFFFFFF;
  AT91C_BASE_TC0->TC_SR;
  AT91C_BASE_TC0->TC_CMR = AT91C_TC_CLKS_TIMER_DIV5_CLOCK | AT91C_TC_CPCTRG;
  AT91C_BASE_TC0->TC_RC = 375;
  AT91C_BASE_TC0->TC_IER = AT91C_TC_CPCS;
  AIC_ConfigureIT(AT91C_ID_TC0, AT91C_AIC_PRIOR_LOWEST, ISR_Timer0);
  AIC_EnableIT(AT91C_ID_TC0);
  AT91C_BASE_TC0->TC_CCR = AT91C_TC_CLKEN | AT91C_TC_SWTRG;

  // Configure timer 1
  AT91C_BASE_PMC->PMC_PCER = (1 << AT91C_ID_TC1);
  AT91C_BASE_TC1->TC_CCR = AT91C_TC_CLKDIS; //Stop clock
  AT91C_BASE_TC1->TC_IDR = 0xFFFFFFFF;    //Disable Interrupts
  AT91C_BASE_TC1->TC_SR;            //Read Status register
  AT91C_BASE_TC1->TC_CMR = AT91C_TC_CLKS_TIMER_DIV4_CLOCK | AT91C_TC_CPCTRG;  // Timer1: 2,666us = 48MHz/128
  AT91C_BASE_TC1->TC_RC = 0xffff;
  AT91C_BASE_TC1->TC_IER = AT91C_TC_CPCS;
  AIC_ConfigureIT(AT91C_ID_TC1, 1, ISR_Timer1);
  AT91C_BASE_TC1->TC_CCR = AT91C_TC_CLKEN | AT91C_TC_SWTRG;

  // Configure timer 1
  AT91C_BASE_PMC->PMC_PCER = (1 << AT91C_ID_TC2);
  AT91C_BASE_TC2->TC_CCR = AT91C_TC_CLKDIS; //Stop clock
  AT91C_BASE_TC2->TC_IDR = 0xFFFFFFFF;    //Disable Interrupts
  AT91C_BASE_TC2->TC_SR;            //Read Status register
  AT91C_BASE_TC2->TC_CMR = AT91C_TC_CLKS_TIMER_DIV4_CLOCK | AT91C_TC_CPCTRG;  // Timer1: 2,666us = 48MHz/128
  AT91C_BASE_TC2->TC_RC = 0xffff;
  AT91C_BASE_TC2->TC_IER = AT91C_TC_CPCS;
  AIC_ConfigureIT(AT91C_ID_TC2, 1, ISR_Timer2);
  AT91C_BASE_TC2->TC_CCR = AT91C_TC_CLKEN | AT91C_TC_SWTRG;

}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
