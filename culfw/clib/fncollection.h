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

#define EE_MAGIC_OFFSET      (uint8_t *)0       // 2 bytes

#define EE_CC1100_CFG        (EE_MAGIC_OFFSET+2)
#define EE_CC1100_CFG_SIZE   0x29       // 41
#define EE_CC1100_PA         (EE_CC1100_CFG+EE_CC1100_CFG_SIZE)
#define EE_CC1100_PA_SIZE    8

#define EE_REQBL             (EE_CC1100_PA+EE_CC1100_PA_SIZE)
#define EE_LED               (EE_REQBL+1)
#define EE_FHTID             (EE_LED+1)

#define EE_FASTRF_CFG        (EE_FHTID+2)
#define EE_FASTRF_CFG_SIZE   0x29       // 41

// EEprom: CUL: 96 bytes of 512 used

#define EE_CONTRAST          (uint8_t *)0x200
#define EE_BRIGHTNESS        (EE_CONTRAST+1)
#define EE_SLEEPTIME         (EE_BRIGHTNESS+1)

// EEprom: CUR: 96+2 bytes of 4096/8192 used

extern uint8_t led_mode;

#endif
