#ifndef _kopp_fc_H
#define _kopp_fc_H
#define MAX_kopp_fc_MSG 15				// (15) typical Kopp transmission block size incl. zeros at the end: 07 FA 5E 10 05 CC 0F 02 DD 00 00 00 00 00 00
#define MAX_kopp_fc_NetMSG 9			// (09) typical Kopp transmission block size without zeros at the end: 07 FA 5E 10 05 CC 0F 02 DD 
#define kopp_fc_Command_char 14			// amount of command line characters for kopp_fc (without "Kt", 2 for Kopp key code, 6 for Transmitter Code, 5 for timeout 1 for J/N)

extern uint8_t kopp_fc_on;
extern uint8_t blkctr; 

/* public kopp function call */
//void kopp_fc_init(void);				// Init not called from external !!
void kopp_fc_task(void);
void kopp_fc_func(char *in);

#endif
