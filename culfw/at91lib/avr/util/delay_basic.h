

#ifndef __DELAY_BASIC_H_
#define __DELAY_BASIC_H_ 1

#include <board.h>
#include <stdint.h>

static inline void _delay_loop_2(uint16_t __count) __attribute__((always_inline));

void
_delay_loop_2(uint16_t __count)
{
	__count>>=1;
	uint8_t i = __count>>8;

	do {
		// Initialize the PIT
		AT91C_BASE_PITC->PITC_PIMR = 2;						//periodic interval value 1µs
		AT91C_BASE_PITC->PITC_PIVR;							//reset overflow counter
		AT91C_BASE_PITC->PITC_PIMR |= AT91C_PITC_PITEN;		//start pit
		while( (AT91C_BASE_PITC->PITC_PIIR >> 20) < (__count & 0xff));
		__count=0xff;
	} while(i--);

}

#endif /* __DELAY_BASIC_H_ */
