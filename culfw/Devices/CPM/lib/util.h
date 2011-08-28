#ifndef UTIL_H_
#define UTIL_H_


#define SET_BIT(PORT, BITNUM) ((PORT) |= (1<<(BITNUM)))
#define CLEAR_BIT(PORT, BITNUM) ((PORT) &= ~(1<<(BITNUM)))
#define TOGGLE_BIT(PORT, BITNUM) ((PORT) ^= (1<<(BITNUM)))


void hardware_init (void);
void led_blink (uint8_t);
uint8_t get_keypress(uint16_t longduration);
void disable_wdi (void);
void enable_wdi (void);
void power_down (void);
void fs20_sendCommand (uint16_t housecode, uint8_t button, uint8_t cmd);
void fs20_sendValue (uint16_t housecode, uint8_t sensor, uint8_t flags, uint16_t value);
void fs20_sendValues (uint16_t housecode, uint8_t sensor, uint16_t value1, uint16_t value2);
void fs20_send3Values (uint16_t housecode, uint8_t sensor, uint16_t value1, uint16_t value2, uint16_t value3);
void reset_clock(void);
uint32_t tick_clock(void);
uint32_t tick_tick(void);
uint32_t get_clock(void);

#endif /* UTIL_H_ */
