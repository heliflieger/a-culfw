#ifndef _DS1339_H
#define _DS1339_H   1

void rtc_init(void);
void rtc_func(char *);
void rtc_get(uint8_t *);
void rtc_set(uint8_t len, uint8_t data[6]);

#endif
