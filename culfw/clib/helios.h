/*  
* Support for Helios
*
* Currently assumes that the RS485 port is connected to the second
* UART (as is the case with the CUNO2)
*  
*/   

#ifndef _HELIOS_H
#define _HELIOS_H

void helios_initialize(void);
void helios_task(void);
void helios_func(char*);

int helios_fs20_emu(char *in);

#endif
