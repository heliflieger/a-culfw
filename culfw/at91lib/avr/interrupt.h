

#ifndef __INTERRUPT_H_
#define __INTERRUPT_H_ 1

#include <stdint.h>

#define cli()		disableIRQ()
#define sei()		enableIRQ()
#define SREG		get_cpsr()

unsigned get_cpsr(void);
unsigned enableIRQ(void);
unsigned disableIRQ(void);
unsigned restoreIRQ(unsigned oldCPSR);


#endif /* __INTERRUPT_H_ */
