#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <stdint.h>
#include <math.h>

// low level functions
// shouldn't be exposed within the application

uint8_t get_config_byte (uint8_t* config);
uint16_t get_config_word (uint16_t* config);
float get_config_float (float* config);

void set_config_byte (uint8_t* config, uint8_t value);
void set_config_word (uint16_t* config, uint16_t value);
void set_config_float (float* config, float value);

#endif
