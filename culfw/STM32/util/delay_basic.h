

#ifndef __DELAY_BASIC_H_
#define __DELAY_BASIC_H_ 1

#include <board.h>
#include <stdint.h>
#include <stm32f1xx.h>
static inline void _delay_loop_2(uint16_t __count) __attribute__((always_inline));

void
_delay_loop_2(uint16_t __count)
{
    volatile uint32_t cycles = (SystemCoreClock/1000000L)*__count>>1;
    volatile uint32_t start = DWT->CYCCNT;
    do  {
    } while(DWT->CYCCNT - start < cycles);


}

#endif /* __DELAY_BASIC_H_ */


