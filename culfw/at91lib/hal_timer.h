
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __hal_timer_H
#define __hal_timer_H

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

void hal_enable_CC_timer_int(uint8_t instance, uint8_t enable);

void HAL_timer_set_reload_register(uint8_t instance, uint32_t value);
uint32_t HAL_timer_get_counter_value(uint8_t instance);
void HAL_timer_reset_counter_value(uint8_t instance);
void HAL_timer_init(void);

#endif /*__ hal_timer_H */
