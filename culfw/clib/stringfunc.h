#ifndef __STRINGFUNC_H
#define __STRINGFUNC_H

#include <avr/pgmspace.h>
#include "board.h"

int fromhex(const char *in, uint8_t *out, uint8_t outlen);
int fromip(const char *in, uint8_t *out, uint8_t outlen);
void fromdec(const char *in, uint8_t *out);
void tohex(uint8_t in, uint8_t *out);
#ifdef ARM
void fromdec32(const char *in, uint32_t *out);
#endif
#endif
