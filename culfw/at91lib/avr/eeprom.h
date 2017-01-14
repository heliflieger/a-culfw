

#ifndef __EEPROM_H_
#define __EEPROM_H_ 1

#include <stdint.h>
#include <board.h>
#include <spi-flash/at45.h>

#define eeprom_busy_wait()
#define E2END	0xfe



void eeprom_write_byte(uint8_t *p, uint8_t v);
uint8_t eeprom_read_byte(uint8_t *p);
uint16_t eeprom_read_word(uint16_t *p);
int16_t flash_init(void);

uint32_t flash_serial(void);

void dump_flash(void);

#endif /* __PGMSPACE_H_ */
