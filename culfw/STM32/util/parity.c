#include "parity.h"

parity_t parity_even_bit (unsigned int x)
{
  parity_t parity;
  unsigned int temp = x;

  for (parity = EVEN; temp > 0; temp >>= 1) {
    parity ^= (temp & 0x1);
  }

  return parity;
}



