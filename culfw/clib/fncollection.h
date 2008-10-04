#ifndef __FNCOLLECTION_H_
#define __FNCOLLECTION_H_

void read_eeprom(char *);
void write_eeprom(char *);
void ledfunc(char *);
void prepare_b(char *);
void prepare_B(char *);
void version(char *);

/* See the README for doc */
#define EE_START_CC1100      (uint8_t *)0x000    // 0x00 - 0x30
#define EE_REQBL             (uint8_t *)0x031

#define EE_LRN_OFF           (uint8_t *)0x033    // First byte to use
#define EE_LRN_SLOTS         23                  // 23*20+0x33 = 511

#define EE_FACT_RESET        (uint8_t *)0x1FF    // Factory reset

#endif
