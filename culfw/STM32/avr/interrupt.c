#include "interrupt.h"


void enableIRQ(void) {
  __asm volatile ("cpsie i" : : : "memory");
}

void disableIRQ(void) {
  __asm volatile ("cpsid i" : : : "memory");
}

void restoreIRQ(unsigned oldCPSR) {
  __asm volatile ("cpsie i" : : : "memory");
}



