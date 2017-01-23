#ifndef __dbgu_H
#define __dbgu_H

extern uint8_t DBGU_RxByte;
extern uint8_t DBGU_RxReady;

void DBGU_init(void);
unsigned int DBGU_IsRxReady(void);
unsigned char DBGU_GetChar(void);
void SWO_PrintChar(char c, uint8_t portNo);

#endif /*__ dbgu_H */


