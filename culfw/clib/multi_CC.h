#ifndef _MULTI_CC_H
#define _MULTI_CC_H

#include <stdint.h>

typedef struct {
  uint8_t instance;
} multiCC_t;

extern multiCC_t multiCC;

void multiCC_func();

#endif
