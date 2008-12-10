#ifndef _DS1339_H
#define _DS1339_H   1

void rtc_init(void);
void rtcfunc(char *);
void rtc_dotime(uint8_t in, uint8_t data[6]);

#endif
