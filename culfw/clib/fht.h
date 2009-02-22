#ifndef __FHT_H
#define __FHT_H

void fhtsend(char *in);
void fht_init(void);
void fht_timer(void);
void fht_hook(uint8_t *in, uint8_t len);
void fht_eeprom_reset(void);

extern uint8_t fht8v_timeout;
extern uint8_t fht_hc[2];


#endif
