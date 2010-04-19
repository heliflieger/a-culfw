#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <stdint.h>

/*
 * configuration offsets
 *
 */

extern volatile uint16_t fs20_housecode;
extern volatile uint8_t  fs20_button;

void readConfig(void);
void saveConfig(uint16_t housecode, uint8_t button);

// low level functions
// shouldn't be exposed within the application
uint8_t	get_config_byte(uint16_t config);
uint16_t get_config_word(uint16_t config);
void get_config_n(uint16_t config, uint8_t *block, uint8_t size);

void set_config_byte(uint16_t config, uint8_t value);
void set_config_word(uint16_t config, uint16_t value);
void set_config_n(uint16_t config, uint8_t *value, uint8_t size);

#endif
