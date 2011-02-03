#include "board.h"

#ifdef HAS_IRRX

#include <avr/io.h>
#include <avr/pgmspace.h>

#include "ir.h"
#include "display.h"
#include "led.h"

#include "irmpconfig.h"
#include "irmp.h"

uint8_t ir_mode = 0;
uint8_t ir_internal = 1;

void ir_init( void ) {

  irmp_init();  // initialize rc5
     
  ir_mode = 0;
  #ifdef HAS_IR_POWER
    IRMP_POWER_DDR |= (1 << IRMP_POWER_BIT); // IRMP_POWER_DDR & BIT als Ausgang festlegen
    IRMP_POWER_PORT |= (1 << IRMP_POWER_BIT); // IRMP_POWER_PORT & BIT aktivieren 
    ir_internal = 1;
  #endif
}

void ir_task( void ) {
  IRMP_DATA irmp_data;
     
  if (!ir_mode)
	  return;
     
  if (!irmp_get_data(&irmp_data))
		return;
     
  if ((ir_mode == 2) && (irmp_data.flags & IRMP_FLAG_REPETITION))
	  return;
     
     
  LED_ON();
  DC('I');
  DH(irmp_data.protocol, 2);
  DH(irmp_data.address, 4);
  DH(irmp_data.command, 4);
  DH(irmp_data.flags, 2);
  DNL();
  LED_OFF();
}

void ir_sample( void ) {
    if (!ir_mode)
	    return;

    (void) irmp_ISR();  // call irmp ISR
}

void ir_func(char *in) {

     switch (in[1]) {
	    case 'r': // receive
			  if (in[2] && in[3]) {
	      	 fromhex(in+2, &ir_mode, 2);
		  	}
		  	DH(ir_mode,2);
		  	DNL();
	  	break;
	  
    	case 's': // send
	  	break;
			
    #ifdef HAS_IR_POWER
      case 'i': // Internal Reciever on/off
        if (ir_internal) {
          IRMP_POWER_PORT &= ~(1 << IRMP_POWER_BIT); // IRMP_POWER_PORT & BIT deaktivieren 
          ir_internal = 0;
        } else {
          IRMP_POWER_PORT |= (1 << IRMP_POWER_BIT); // IRMP_POWER_PORT & BIT  aktivieren 
          ir_internal = 1;
        }
        DH(ir_internal,2);
  	    DNL();  		
	  	break;
    #endif
	  
    }
     

}

#endif