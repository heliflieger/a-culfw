#ifndef __ringbuffer_H_
#define __ringbuffer_H_

#include <stdint.h>

typedef struct
{   
  uint8_t size;         // Buffer length
  uint8_t putoff;
  uint8_t getoff;
  uint8_t nbytes;       // Number of data bytes
  char buf[];           // Buffer begins here
} rb_t;
    
#define DEFINE_RBUF(name,size)\
          uint8_t name##_data[size+4];\
          rb_t *const name = (rb_t *)name##_data;
void rb_init(rb_t *rb, uint8_t size);
void rb_put(rb_t *rb, uint8_t data);
uint8_t rb_get(rb_t *rb);
void rb_reset(rb_t *rb);

#endif
