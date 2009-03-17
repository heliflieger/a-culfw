#ifndef __MENU_H
#define __MENU_H

void menu_init(void);
void menu_push(uint8_t idx);
void menu_redisplay(void);
void menu_handle_joystick(uint8_t key);

#endif
