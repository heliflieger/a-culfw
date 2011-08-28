#include "config.h"
#include <avr/eeprom.h>
#include <stdint.h>
#include <math.h>

/*
 * defining your configuration variables:
 * 
 * uint8_t yourByteVariable EEMEM;
 * uint16_t yourWordVariable EEMEM;
 * float yourFloatVariable EEMEM;
 * 
 * 
 * reading a variable:
 * 
 * yourByteData= get_config_byte(&yourByteVariable);
 * ...
 * 
 * 
 * writing a variable:
 * set_config_byte(&yourByteVariable, 42);
 * 
 * 
 * see also: http://www.mikrocontroller.net/articles/AVR-GCC-Tutorial#EEPROM
 * 
 */

/*
 * Configuration
 */

uint8_t
get_config_byte (uint8_t* config)
{
  return eeprom_read_byte(config);
}

uint16_t
get_config_word (uint16_t* config)
{
  return eeprom_read_word(config);
}

float
get_config_float (float* config)
{
  return eeprom_read_float(config);
}

void
set_config_byte (uint8_t* config, uint8_t value)
{
  eeprom_write_byte(config, value);
}

void
set_config_word (uint16_t* config, uint16_t value)
{
  eeprom_write_word(config, value);
}

void
set_config_float (float* config, float value)
{
  eeprom_write_float(config, value);
}
