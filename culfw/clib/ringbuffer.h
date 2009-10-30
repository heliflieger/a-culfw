#ifndef __ringbuffer_H_
#define __ringbuffer_H_

#include "board.h"
#include <stdint.h>

typedef struct
{   
  uint8_t putoff;
  uint8_t getoff;
  uint8_t nbytes;       // Number of data bytes
  char buf[TTY_BUFSIZE];
} rb_t;
    
void rb_put(rb_t *rb, uint8_t data);
uint8_t rb_get(rb_t *rb);
void rb_reset(rb_t *rb);

#endif
