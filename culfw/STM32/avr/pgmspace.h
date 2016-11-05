

#ifndef __PGMSPACE_H_
#define __PGMSPACE_H_ 1


#define PROGMEM
#include <stdint.h>
#include <string.h>

#define __LPM(addr)         *(addr)
#define __LPM_word(addr)    *(addr)
#define __LPM_dword(addr)   *(addr)
#define __LPM_float(addr)   *(addr)

#define PSTR(s) s

#define strcpy_P	strcpy

#define pgm_read_byte(address_short)    (*address_short)

#define PGM_P const char *

#endif /* __PGMSPACE_H_ */
