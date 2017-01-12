//#define TESTING
#ifdef TESTING
#define TTY_BUFSIZE 64
typedef unsigned char uint8_t;
#include <stdio.h>
#endif

#include <avr/interrupt.h>              // for cli
#include <avr/io.h>                     // for SREG
#include <stdint.h>                     // for uint8_t

#include "ringbuffer.h"

#ifndef ARM
#define restoreIRQ(sr)		SREG = sr
#endif

void
rb_reset(rb_t *rb)
{
  rb->getoff = rb->putoff = rb->nbytes = 0;
}

void
rb_put(rb_t *rb, uint8_t data)
{
  uint8_t sreg;
  sreg = SREG;
  cli();
  if(rb->nbytes >= TTY_BUFSIZE) {
	  restoreIRQ(sreg);
    return;
  }
  rb->nbytes++;
  rb->buf[rb->putoff++] = data;
  if(rb->putoff == TTY_BUFSIZE)
    rb->putoff = 0;
  restoreIRQ(sreg);
}

uint8_t
rb_get(rb_t *rb)
{
  uint8_t sreg;
  uint8_t ret;
  sreg = SREG;
  cli();
  if(rb->nbytes == 0) {
	restoreIRQ(sreg);
    return 0;
  }
  rb->nbytes--;
  ret = rb->buf[rb->getoff++];
  if(rb->getoff == TTY_BUFSIZE)
    rb->getoff = 0;
  restoreIRQ(sreg);
  return ret;
}

#ifdef TESTING
void
d_s(rb_t *rb, char *s)
{
  while(*s)
    rb_put(rb, *s++);
}

void
p(rb_t *rb)
{
  uint8_t c;
  while((c = rb_get(rb))) {
    putchar(c);
  }
  putchar('\n');
}


int
main(int ac, char **av)
{
  rb_t buffer;
  rb_reset(&buffer);
  //d_s(&buffer, "T40484269E72E;T40484369001F;");
  d_s(&buffer, "T40484269E743001F;T404847690118;");
  FHT_compress(&buffer);
  p(&buffer);
}
#endif
