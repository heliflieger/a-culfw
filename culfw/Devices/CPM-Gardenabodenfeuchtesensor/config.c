#include "config.h"
#include <avr/eeprom.h>
#include <stdint.h>
#include <avr/pgmspace.h>

uint16_t eefs20_housecode EEMEM = 0x0000;
uint8_t  eefs20_button	  EEMEM = 0x00;

volatile uint16_t fs20_housecode = 0x1234;
volatile uint8_t  fs20_button    = 0x56;

void readConfig(void)
{
    // init variables from eeprom
    uint16_t cfg_housecode = get_config_word(&eefs20_housecode);
    uint8_t  cfg_button	= get_config_byte(&eefs20_button);
    // check if any information available
    if(cfg_housecode != 0xffff)
    {
    	fs20_housecode = cfg_housecode;
    	fs20_button = cfg_button;
    }
}

void saveConfig(uint16_t housecode, uint8_t button)
{
	fs20_housecode = housecode;
	fs20_button = button;

	// saves the housecode and buttoncode to the eeprom
	set_config_word(&eefs20_housecode, fs20_housecode);
	set_config_byte(&eefs20_button, fs20_button);
}

/*
 * EEPROM functionality
 */

__attribute__((__noinline__))
void eeprom_w1(uint16_t addr, uint8_t byte) {
	eeprom_write_byte((uint8_t*)addr, byte);
}

__attribute__((__noinline__))
uint8_t eeprom_r1(uint16_t addr) {
	return eeprom_read_byte((uint8_t*)addr);
}

__attribute__((__noinline__))
void eeprom_w2(uint16_t addr, uint16_t word) {
	eeprom_write_word((uint16_t*)addr, word);
}

__attribute__((__noinline__))
uint16_t eeprom_r2(uint16_t addr) {
	return eeprom_read_word((uint16_t*)addr);
}

/*
__attribute__((__noinline__))
void eeprom_wn(uint16_t addr, uint8_t *block, uint8_t size) {
	eeprom_write_block((uint16_t*)addr, block, size);
}

__attribute__((__noinline__))
void eeprom_rn(uint16_t addr, uint8_t *block, uint8_t size) {
	eeprom_read_block(block, (uint16_t*)addr, size);
}
*/

/*
 * Configuration
 */


uint8_t	get_config_byte(uint16_t config) {
	return eeprom_r1(config);
}

uint16_t get_config_word(uint16_t config) {
	return eeprom_r2(config);
}

void get_config_n(uint16_t config, uint8_t *block, uint8_t size) {
	for(uint8_t i= 0; i< size; i++) block[i]= eeprom_r1(config+i);
	//return eeprom_rn(config, block, size);
}

void set_config_byte(uint16_t config, uint8_t value) {
	eeprom_w1(config, value);
}

void set_config_word(uint16_t config, uint16_t value) {
	eeprom_w2(config, value);
}

void set_config_n(uint16_t config, uint8_t *value, uint8_t size) {
	for(uint8_t i= 0; i< size; i++) eeprom_w1(config+i, value[i]);
	//eeprom_wn(config, value, size);
}


