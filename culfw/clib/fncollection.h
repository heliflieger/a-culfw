#ifndef __FNCOLLECTION_H_
#define __FNCOLLECTION_H_

void read_eeprom(char *);
void write_eeprom(char *);
void ledfunc(char *);
void prepare_b(char *);
void prepare_B(char *);
void version(char *);
void lcdout(char *in);
void timer(char *in);

/* See the README for doc */

#define EE_START_CC1100      (uint8_t *)0x000    // 0x00 - 0x30
#define EE_REQBL             (uint8_t *)0x031
#define EE_START_MENU        (uint8_t *)0x032   // 0x032 - 0x051
#define EE_CONTRAST          (uint8_t *)0x052

#define EE_START_UNUSED      (uint8_t *)0x053
#define EE_FACT_RESET        (uint8_t *)0x1FF    // Factory reset

extern uint8_t led_mode;

#endif
