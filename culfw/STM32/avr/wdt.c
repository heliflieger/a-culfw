
#include "wdt.h"
#include <board.h>
#include "stm32f1xx_hal.h"

extern void Error_Handler(void);
extern IWDG_HandleTypeDef hiwdg;

IWDG_HandleTypeDef hiwdg;

void wdt_reset(void) {
  HAL_IWDG_Refresh(&hiwdg);
}

void wdt_enable(uint16_t reload) {
  hiwdg.Instance = IWDG;
  hiwdg.Init.Prescaler = IWDG_PRESCALER_32;
  hiwdg.Init.Reload = reload;
  if (HAL_IWDG_Init(&hiwdg) != HAL_OK)
  {
    Error_Handler();
  }
  HAL_IWDG_Start(&hiwdg);
}

void wdt_disable() {

}
