#ifndef _HW_AUTODETECT_H
#define _HW_AUTODETECT_H

void hw_autodetect(void);
uint8_t has_CC(uint8_t num);
uint8_t has_ethernet(void);
uint8_t get_hw_features(void);
uint8_t has_onewire(void);

#endif
