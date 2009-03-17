#ifndef __STRINGFUNC_H
#define __STRINGFUNC_H

#include <avr/pgmspace.h>

int fromhex(const char *in, uint8_t *out, uint8_t fromlen);
void tohex(uint8_t in, uint8_t *out);

#endif
