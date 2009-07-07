#ifndef _BATTERY_H
#define _BATTERY_H

void batfunc(char *unused);
void bat_init(void);
void bat_drawstate(void);
extern uint8_t battery_state;

#endif
