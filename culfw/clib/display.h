#ifndef __DISPLAY_H_
#define __DISPLAY_H_

#include <stdint.h>
#include <avr/pgmspace.h>

int fromhex(const char *in, uint8_t *out, uint8_t fromlen);
void display_char(char s);
void display_string(char *s);
void display_string_P(prog_char *s);
void display_udec(uint16_t d, int8_t pad, uint8_t padc);
void display_hex(uint16_t h, int8_t pad, uint8_t padc);
void display_nl(void);

#define DC display_char
#define DS display_string
#define DS_P display_string_P
#define DU(a,b) display_udec(a,b,' ')
#define DH(a,b) display_hex(a,b,'0')
#define DNL display_nl

#endif
