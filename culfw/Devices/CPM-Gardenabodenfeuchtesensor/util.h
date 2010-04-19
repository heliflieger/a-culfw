/*
 * util.h
 *
 *  Created on: 12.04.2010
 *      Author: maz
 */

#ifndef UTIL_H_
#define UTIL_H_

static inline void SetSystemClockPrescaler(uint8_t PrescalerMask) {
     uint8_t tmp = (1 << CLKPCE);
     __asm__ __volatile__ (
	  "in __tmp_reg__,__SREG__" "\n\t"
	  "cli" "\n\t"
	  "sts %1, %0" "\n\t"
	  "sts %1, %2" "\n\t"
	  "out __SREG__, __tmp_reg__"
	  : /* no outputs */
	  : "d" (tmp),
	  "M" (_SFR_MEM_ADDR(CLKPR)),
	  "d" (PrescalerMask)
	  : "r0");
}

static inline void led_blink(uint8_t num) {
	// save current led state
	uint8_t oldstate = LED_PORT & _BV(LED_PIN);
	for(uint8_t i=0; i<num*2; i++) {
		LED_TOG();
		_delay_ms(100);
	}
	// reset old state
	if(oldstate)
		LED_ON();
	else
		LED_OFF();
}


#endif /* UTIL_H_ */
