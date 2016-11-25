#include "interrupt.h"

#define IRQ_MASK 0x000000C0


static inline unsigned asm_get_cpsr(void) {
	unsigned long retval;
	__asm volatile (" mrs %0, cpsr" : "=r" (retval) : /* no inputs */ );
	return retval;
}

static inline void asm_set_cpsr(unsigned val) {
	__asm volatile (" msr cpsr, %0" : /* no outputs */ : "r" (val) );
}

unsigned get_cpsr(void) {
	return asm_get_cpsr();
}
unsigned enableIRQ(void) {
	unsigned _cpsr;
	_cpsr = asm_get_cpsr();
	asm_set_cpsr(_cpsr & ~IRQ_MASK);
	return _cpsr;
}

unsigned disableIRQ(void) {
	unsigned _cpsr;
	_cpsr = asm_get_cpsr();
	asm_set_cpsr(_cpsr | IRQ_MASK);
	return _cpsr;
}

unsigned restoreIRQ(unsigned oldCPSR) {
	unsigned _cpsr;
	_cpsr = asm_get_cpsr();
	asm_set_cpsr((_cpsr & ~IRQ_MASK) | (oldCPSR & IRQ_MASK));
	return _cpsr;
}



