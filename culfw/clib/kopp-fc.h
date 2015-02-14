#ifndef _kopp_fc_H
#define _kopp_fc_H
#define MAX_kopp_fc_MSG 14
#define kopp_fc_Command_char 14			// amount of command line characters for kopp_fc (without "Kt", 2 for Kopp key code, 6 for Transmitter Code, 5 for timeout)
extern uint8_t kopp_fc_on;
extern uint8_t blkctr; 

/* public kopp function call */
void kopp_fc_init(void);
// void kopp_fc_task(void);
void kopp_fc_func(char *in);

#endif
