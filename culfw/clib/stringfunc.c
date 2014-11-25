#include "stringfunc.h"

/*
 * Converts a hex string to a buffer. Not hex characters will be skipped
 * Returns the hex bytes found. Single-Nibbles wont be converted.
 */
int
fromhex(const char *in, uint8_t *out, uint8_t buflen)
{
  uint8_t *op = out, c, h = 0, step = 0;
  while((c = *in++)) {
    if(c >= 'a')
      c -= 'a'-'A';
    if(c >= '0' && c <= '9') {
      h |= c-'0';
    } else if(c >= 'A' && c <= 'F') {
      h |= c-'A'+10;
    } else  {
      if(c != ' ' && c != ':')
        break;
      continue;
    }
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

// Used to parse ip-adresses, but may also be used to parse single-byte values
int
fromip(const char *in, uint8_t *out, uint8_t buflen)
{
  uint8_t *op = out, c, h = 0, fnd = 0;
  while((c = *in++)) {
    fnd = 0;
    if(c >= '0' && c <= '9') {
      h = h*10 + (c-'0');
      fnd = 1;
    } else {
      if(c != ' ' && c != '.')
        break;
      if(buflen) {
        *op++ = h;
        fnd = h = 0;
        buflen--;
      }
    }
  }
  if(fnd && buflen)
    *op++ = h;
  return op-out;
}

// Used to parse ip-adresses, but may also be used to parse single-byte values
void
fromdec(const char *in, uint8_t *out)
{
  uint8_t c;
  uint16_t h = 0;

  while((c = *in++))
    if(c >= '0' && c <= '9')
      h = h*10 + (c-'0');
  *(uint16_t*)out = h;
}


// Just one byte
void
tohex(uint8_t f, uint8_t *t)
{
  uint8_t m = (f>>4)&0xf;
  t[0] = (m < 10 ? '0'+m : 'A'+m-10);
  m = f & 0xf;
  t[1] = (m < 10 ? '0'+m : 'A'+m-10);
}
