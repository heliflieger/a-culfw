
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __hal_timer_H
#define __hal_timer_H

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

#define HAL_TIMER_SET_RELOAD_REGISTER(x)    ((AT91C_BASE_TC1->TC_RC) = (x))
#define HAL_TIMER_GET_COUNTER_VALUE()       (AT91C_BASE_TC1->TC_CV)
#define HAL_TIMER_RESET_COUNTER_VALUE()     (AT91C_BASE_TC1->TC_CCR = AT91C_TC_CLKEN | AT91C_TC_SWTRG)


void hal_enable_CC_timer_int(uint8_t enable);


#endif /*__ hal_timer_H */
