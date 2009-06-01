#ifndef __FHT_H
#define __FHT_H

void fhtsend(char *in);
void fht_init(void);
void fht_hook(uint8_t *in);
void fht8v_timer(void);
void fht80b_timer(void);

extern uint8_t fht8v_timeout;
extern uint8_t fht80b_timeout;
extern uint8_t fht_hc[2];


#endif
