#ifndef _DS1339_H
#define _DS1339_H   1

void rtc_init(void);
void rtcfunc(char *);
void rtcget(uint8_t *);
void rtcset(uint8_t len, uint8_t data[6]);

#endif
