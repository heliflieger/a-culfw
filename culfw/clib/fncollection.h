#ifndef __FNCOLLECTION_H_
#define __FNCOLLECTION_H_

void read_eeprom(char *);
void write_eeprom(char *);
void eeprom_init(void);
void eeprom_factory_reset(char *unused);

void ledfunc(char *);
void prepare_boot(char *);
void version(char *);
void monitor(char *in);

/* See the README for doc */

#define EE_MAGIC             'c'
#define EE_VERSION           '1'

#define EE_MAGIC_OFFSET      (uint8_t *)0x000
#define EE_VERSION_OFFSET    (uint8_t *)0x001

#define EE_START_CC1100      (uint8_t *)0x002   // 49: 0x002 - 0x02B - 0x032
#define EE_REQBL             (uint8_t *)0x033   // Request bootloader
#define EE_START_MENU        (uint8_t *)0x034   // 64: 0x034 - 0x073
#define EE_CONTRAST          (uint8_t *)0x074
#define EE_BRIGHTNESS        (uint8_t *)0x075
#define EE_LED               (uint8_t *)0x076
#define EE_SLEEPTIME         (uint8_t *)0x077

#define EE_START_UNUSED      (uint8_t *)0x078


extern uint8_t led_mode;

#endif
