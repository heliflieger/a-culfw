#ifndef UTIL_H_
#define UTIL_H_


#define SET_BIT(PORT, BITNUM) ((PORT) |= (1<<(BITNUM)))
#define CLEAR_BIT(PORT, BITNUM) ((PORT) &= ~(1<<(BITNUM)))
#define TOGGLE_BIT(PORT, BITNUM) ((PORT) ^= (1<<(BITNUM)))


void hardware_init (void);
void led_blink (uint8_t);
uint8_t get_keypress(uint16_t longduration);
void enable_wdi (void);
void power_down (void);
void fs20_sendCommand (uint16_t housecode, uint8_t button, uint8_t cmd);
void fs20_sendValue (uint16_t housecode, uint8_t sensor, uint8_t flags, uint16_t value);
void fs20_sendValues (uint16_t housecode, uint8_t sensor, uint16_t value1, uint16_t value2);
void reset_clock(void);
uint32_t tick_clock(void);
uint32_t get_clock(void);

static inline void SetSystemClockPrescaler(uint8_t PrescalerMask) {
     uint8_t tmp = (1 << CLKPCE);
     __asm__ __volatile__ (
          "in __tmp_reg__,__SREG__" "\n\t"
          "cli" "\n\t"
          "sts %1, %0" "\n\t"
          "sts %1, %2" "\n\t"
          "out __SREG__, __tmp_reg__"
          : /* no outputs */
          : "d" (tmp),
          "M" (_SFR_MEM_ADDR(CLKPR)),
          "d" (PrescalerMask)
          : "r0");
}


#endif /* UTIL_H_ */
