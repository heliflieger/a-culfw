#ifndef __DISPLAY_H_
#define __DISPLAY_H_

#include <stdint.h>
#include <avr/pgmspace.h>
#include "stringfunc.h"

void display_char(char s);
void display_string(char *s);
void display_string_P(const char *s);
void display_udec(uint16_t d, int8_t pad, uint8_t padc);
void display_hex(uint16_t h, int8_t pad, uint8_t padc);
void display_hex2(uint8_t h);
void display_nl(void);

#define DC display_char
#define DS display_string
#define DS_P display_string_P
#define DU(a,b) display_udec(a,b,' ')
#define DH(a,b) display_hex(a,b,'0')
#define DH2(a) display_hex2(a)
#define DNL display_nl

extern uint8_t display_channel;
extern uint8_t log_enabled;

#define DISPLAY_USB      (1<<0)
#define DISPLAY_LCD      (1<<1)
#define DISPLAY_TCP      (1<<2)
#define DISPLAY_RFROUTER (1<<3)
#define DISPLAY_DOGM     (1<<4)


#endif
