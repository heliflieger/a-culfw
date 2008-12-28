#ifndef __FNCOLLECTION_H_
#define __FNCOLLECTION_H_

void read_eeprom(char *);
void write_eeprom(char *);
void eeprom_init(void);
void eeprom_factory_reset(char *unused);
void ewb(uint8_t *p, uint8_t v);
uint8_t erb(uint8_t *p);

void ledfunc(char *);
void prepare_boot(char *);
void version(char *);

// Already used magics: c1,c2

#define EE_MAGIC             'c'
#define EE_VERSION           '3'

#define EE_MAGIC_OFFSET      (uint8_t *)0
#define EE_VERSION_OFFSET    (uint8_t *)1

#define EE_START_CC1100      (uint8_t *)2
#define EE_CC1100_SIZE1      0x29       // 41
#define EE_CC1100_SIZE       (EE_CC1100_SIZE1+8)

#define EE_REQBL             (uint8_t *)(EE_START_CC1100+EE_CC1100_SIZE)

#define EE_FHTID             (uint8_t *)(EE_REQBL+1)
#define EE_FHT_ACTUATORS     (uint8_t *)(EE_FHTID+2)
#define EE_START_FHTBUF      (uint8_t *)(EE_FHT_ACTUATORS+18)
#define EE_FHTSLOTS          50
#define EE_FHTBUF_SIZE       (5*EE_FHTSLOTS)     // 5 byte each, must be <= 255

// EEprom: CUL: 322 bytes of 512 used

#define EE_START_MENU        (uint8_t *)0x200
#define EE_MENU_SIZE         0x40          // Last position for 64 menu entries
#define EE_CONTRAST          (uint8_t *)(EE_START_MENU+EE_MENU_SIZE)
#define EE_BRIGHTNESS        (uint8_t *)(EE_CONTRAST+1)
#define EE_LED               (uint8_t *)(EE_BRIGHTNESS+1)
#define EE_SLEEPTIME         (uint8_t *)(EE_LED+1)

#define EE_START_UNUSED      (uint8_t *)0x078

// EEprom: CUR: 580 bytes of 4096 used, minus the hole at the CUL

extern uint8_t led_mode;

#endif
