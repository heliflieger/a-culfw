

#ifndef __INTERRUPT_H_
#define __INTERRUPT_H_ 1

#include <stdint.h>

#define cli()		disableIRQ()
#define sei()		enableIRQ()
#define SREG		1

//unsigned get_cpsr(void);
void enableIRQ(void);
void disableIRQ(void);
void restoreIRQ(unsigned oldCPSR);


#endif /* __INTERRUPT_H_ */
