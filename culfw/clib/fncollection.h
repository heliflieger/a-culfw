#ifndef __FNCOLLECTION_H_
#define __FNCOLLECTION_H_

void read_eeprom(char *);
void write_eeprom(char *);
void eeprom_init(void);
void eeprom_factory_reset(char *unused);

void ledfunc(char *);
void prepare_boot(char *);
void version(char *);

/* See the README for doc */
// Already used magic: c1

#define EE_MAGIC             'c'
#define EE_VERSION           '2'

#define EE_MAGIC_OFFSET      (uint8_t *)0
#define EE_VERSION_OFFSET    (uint8_t *)1

#define EE_START_CC1100      (uint8_t *)2
#define EE_CC1100_SIZE       (0x29+0x08)

#define EE_REQBL             (uint8_t *)(EE_START_CC1100+EE_CC1100_SIZE)
#define EE_CONTRAST          (uint8_t *)(EE_REQBL+1)
#define EE_BRIGHTNESS        (uint8_t *)(EE_CONTRAST+1)
#define EE_LED               (uint8_t *)(EE_BRIGHTNESS+1)
#define EE_SLEEPTIME         (uint8_t *)(EE_LED+1)

#define EE_START_MENU        (uint8_t *)0x200

#define EE_START_UNUSED      (uint8_t *)0x078


extern uint8_t led_mode;

#endif
