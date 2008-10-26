#include "ringbuffer.h"

void
rb_init(rb_t *rb, uint8_t size)
{
  rb->size = size;
  rb->getoff = rb->putoff = rb->nbytes = 0;
}

void
rb_reset(rb_t *rb)
{
  rb->getoff = rb->putoff = rb->nbytes = 0;
}

void
rb_put(rb_t *rb, uint8_t data)
{
  if(rb->nbytes >= rb->size) {
    return;
  }
  rb->nbytes++;
  rb->buf[rb->putoff++] = data;
  if(rb->putoff == rb->size)
    rb->putoff = 0;
}

uint8_t
rb_get(rb_t *rb)
{
  uint8_t ret;
  if(rb->nbytes == 0)
    return 0;
  rb->nbytes--;
  ret = rb->buf[rb->getoff++];
  if(rb->getoff == rb->size)
    rb->getoff = 0;
  return ret;
}
