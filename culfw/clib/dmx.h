/* 
 * Digital AC Dimmer 
 * 
 * Copyright (c) 2005-2006 Chris Fallin <chris@cfallin.org> 
 * Placed under the modified BSD license 
 * 
 * dmx.h: DMX-512 transmitter module 
 */ 
 
#ifndef _DMX_H_ 
#define _DMX_H_ 
 
void dmx_initialize(int channel_count); 
 
// 'start' differs from 'initialize' in that 'start' should be called after 
// everything's set up and interrupts are enabled, in order to start off the 
// chain of interrupts that transmits the DMX stream 
void dmx_start(void); 
 
void dmx_set_level(int channel, char level); 
 
void dmx_func(char *in);
int dmx_fs20_emu(char *in);

#endif 
