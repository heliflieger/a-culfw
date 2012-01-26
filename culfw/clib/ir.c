/* Copyright DHS-Computertechnik GmbH, 
   Olaf Droegehorn, 2011.
   Released under the GPL Licence, Version 2
*/

#include "board.h"

#if defined (HAS_IRRX) || defined (HAS_IRTX)

#include <avr/io.h>
#include <avr/pgmspace.h>

#include "ir.h"
#include "display.h"
#include "led.h"

#include "irmpconfig.h"
#include "irsndconfig.h"
#include "irmp.h"
#include "irsnd.h"

uint8_t ir_mode = 0;
uint8_t ir_internal = 1;

void ir_init( void ) {

#ifdef HAS_IRRX
  irmp_init();  // initialize rc5
     
  ir_mode = 0;
  #ifdef HAS_IR_POWER
    IRMP_POWER_DDR |= (1 << IRMP_POWER_BIT); // IRMP_POWER_DDR & BIT als Ausgang festlegen
    IRMP_POWER_PORT |= (1 << IRMP_POWER_BIT); // IRMP_POWER_PORT & BIT aktivieren 
    ir_internal = 1;
  #endif
#endif 
 
#ifdef HAS_IRTX
  irsnd_init();
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

uint8_t ir_send_data (void) {
	return irsnd_ISR();
}

void ir_func(char *in) {
#ifdef HAS_IRTX 
   	 IRMP_DATA irmp_data;
   	 uint8_t higha = 0, lowa = 0, highc = 0, lowc = 0, protocol = 0, flags  = 0;
   	 uint16_t address = 0, command = 0;
#endif   	 
   	 
     switch (in[1]) {
    #ifdef HAS_IRRX
	    case 'r': // receive
			  if (in[2] && in[3]) {
	      	 fromhex(in+2, &ir_mode, 2);
		  	}
		  	DH(ir_mode,2);
		  	DNL();
	  	break;
    #endif	  

    #ifdef HAS_IRTX 
    	case 's': // send
	      fromhex(in+2, &protocol, 1);
	      fromhex(in+4, &higha, 1);
	      fromhex(in+6, &lowa, 1);
	      address = higha*256+lowa;
	      fromhex(in+8, &highc, 1);
	      fromhex(in+10, &lowc, 1);
	      command = highc*256+lowc;
	      fromhex(in+12, &flags, 1);

        irmp_data.protocol = protocol;
        irmp_data.address  = address;
        irmp_data.command  = command;
        irmp_data.flags    = flags;

	      irsnd_send_data (&irmp_data, TRUE);

	  	break;
    #endif	  
			
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