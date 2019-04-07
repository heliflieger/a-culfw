/**
  ******************************************************************************
  * File Name          : gpio.h
  * Description        : This file contains all the functions prototypes for 
  *                      the gpio  
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __gpio_H
#define __gpio_H


/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"
#include "board.h"

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#define INIT_MODE_OUT_CS_IN    0
#define INIT_MODE_IN_CS_IN     1

typedef enum
{
  LED0 = 0,
#ifdef LED2_GPIO
  LED1,
#endif
#ifdef LED3_GPIO
  LED2,
#endif
  LED_COUNT
} LED_List;

typedef enum
{
  LED_off = 0,
  LED_on
} LED_State;

typedef struct {
  GPIO_TypeDef*  base[3];
  uint8_t       pin[3];
} transceiver_t;

typedef enum {
  CC_Pin_Out = 0,
  CC_Pin_CS,
  CC_Pin_In
} CC_PIN;

extern transceiver_t CCtransceiver[];
extern uint8_t i2cPort;

void MX_GPIO_Init(void);

void hal_UCBD_connect_init(void);

void HAL_LED_Init(void);
void HAL_LED_Set(LED_List led, LED_State state);
void HAL_LED_Toggle(LED_List led);

void hal_CC_GDO_init(uint8_t cc_num, uint8_t mode);
void hal_enable_CC_GDOin_int(uint8_t cc_num, uint8_t enable);
void hal_CC_Pin_Set(uint8_t cc_num, CC_PIN pin, GPIO_PinState state);
uint32_t hal_CC_Pin_Get(uint8_t cc_num, CC_PIN pin);
void hal_CC_move_transceiver_pins(uint8_t source, uint8_t dest);
void hal_I2C_Set_Port(uint8_t port);

void hal_GPIO_EXTI_IRQHandler(void);

void hal_wiznet_Init(void);

#endif /*__ pinoutConfig_H */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
