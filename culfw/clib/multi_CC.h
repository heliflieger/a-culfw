#ifndef _MULTI_CC_H
#define _MULTI_CC_H

#include <stdint.h>
#include "rf_mode.h"

#ifdef HAS_MULTI_CC

#define MULTICC_PREFIX()  multiCC_prefix()

void multiCC_prefix(void);
void multiCC_func();

#else

#define MULTICC_PREFIX()  {}

#endif

#endif
