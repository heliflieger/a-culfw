/*  
* Support for the RS485 based "HM485" protocol (also called Homematic Wired)
*
* Currently assumes that the RS485 port is connected to the second
* UART (as is the case with the CUNO2)
*  
* Written by Oliver Wagner <owagner@vapor.com>
* Placed under the modified BSD license  
*  
*/   

#ifndef _HM485_H
#define _HM485_H

void hm485_initialize(void);
void hm485_task(void);
void hm485_func(char*);

#endif
