
#ifndef __gpio_H
#define __gpio_H


/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
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
#ifdef PIN_LED_1
  LED1,
#endif
#ifdef PIN_LED_2
  LED2,
#endif
  LED_COUNT
}LED_List;

typedef enum {
  LED_off = 0,
  LED_on
}LED_State;

typedef enum {
  GPIO_PIN_RESET = 0,
  GPIO_PIN_SET
}GPIO_PinState;

typedef struct {
  AT91PS_PIO  base[3];
  uint8_t     pin[3];
}transceiver_t;

typedef enum {
  CC_Pin_Out = 0,
  CC_Pin_CS,
  CC_Pin_In
}CC_PIN;

extern transceiver_t CCtransceiver[];
extern uint8_t i2cPort;

void HAL_GPIO_Init(void);

//void hal_UCBD_connect_init(void);

void HAL_LED_Init(void);
void HAL_LED_Set(LED_List led, LED_State state);
void HAL_LED_Toggle(LED_List led);

void hal_CC_GDO_init(uint8_t cc_num, uint8_t mode);
void hal_enable_CC_GDOin_int(uint8_t cc_num, uint8_t enable);
void hal_CC_Pin_Set(uint8_t cc_num, CC_PIN pin, GPIO_PinState state);
uint32_t hal_CC_Pin_Get(uint8_t cc_num, CC_PIN pin);
void hal_CC_move_transceiver_pins(uint8_t source, uint8_t dest);
void hal_I2C_Set_Port(uint8_t port);

//void hal_GPIO_EXTI_IRQHandler(void);


#endif
