#include "board.h"
#include "display.h"
#include "ringbuffer.h"
#include "cdc.h"
#include "led.h"
#include "delay.h"
#include "pcf8833.h"

#if defined(USB_OPTIONAL)
uint8_t display_channels = DISPLAY_USB; // GLCD will be switched on
#endif

/*
 * Converts a hex string to a buffer. Not hex characters will be skipped
 * Returns the hex bytes found. Single-Nibbles wont be converted.
 */
int
fromhex(const char *in, uint8_t *out, uint8_t buflen)
{
  uint8_t *op = out, c, h = 0, fnd, step = 0;
  while((c = *in++)) {
    fnd = 0;
    if(c >= '0' && c <= '9') { h |= c-'0';    fnd = 1; }
    if(c >= 'A' && c <= 'F') { h |= c-'A'+10; fnd = 1; }
    if(c >= 'a' && c <= 'f') { h |= c-'a'+10; fnd = 1; }
    if(!fnd)
      continue;
    if(step++) {
      *op++ = h;
      if(--buflen <= 0)
        return (op-out);
      step = 0;
      h = 0;
    } else {
      h <<= 4;
    }
  }
  return op-out;
}

#if defined(USB_OPTIONAL)
#define CHECK(a) if(display_channels & a)
#else
#define CHECK(a)
#endif

//////////////////////////////////////////////////
// Display routines
void
display_char(char data)
{
#ifdef HAS_USB
  CHECK(DISPLAY_USB)
  {
    if(USB_Tx_Buffer->nbytes >= USB_Tx_Buffer->size)
      CDC_Task();
    rb_put(USB_Tx_Buffer, data);
  }
#endif

#ifdef HAS_GLCD
  CHECK(DISPLAY_GLCD)
  {
    static uint8_t buf[20];
    static uint8_t off = 2;

    if(data == '\r')
      return;
    if(data == '\n') {
      buf[off] = 0;
      buf[1] = '0';
      buf[2] = '9';
      lcdfunc((char *)buf);
      off = 3;
      return;
    } else {
      if(off < 19)
        buf[off++] = data;
    }
  }
#endif
}

void
display_string(char *s)
{
  while(*s)
    display_char(*s++);
}

void
display_string_P(prog_char *s)
{
  uint8_t c;
  while((c = __LPM(s))) {
    display_char(c);
    s++;
  }
}

void
display_nl()
{
  display_char('\r');
  display_char('\n');
}

void
display_udec(uint16_t d, int8_t pad, uint8_t padc)
{
  char buf[6];
  int8_t i=6;

  buf[--i] = 0;
  do {
    buf[--i] = d%10 + '0';
    d /= 10;
    pad--;
  } while(d && i);

  while(--pad >= 0 && i > 0)
    buf[--i] = padc;
  DS(buf+i);
}

void
display_hex(uint16_t h, int8_t pad, uint8_t padc)
{
  char buf[5];
  int8_t i=5;

  buf[--i] = 0;
  do {
    uint8_t m = h%16;
    buf[--i] = (m < 10 ? '0'+m : 'A'+m-10);
    h /= 16;
    pad--;
  } while(h);

  while(--pad >= 0 && i > 0)
    buf[--i] = padc;
  DS(buf+i);
}
