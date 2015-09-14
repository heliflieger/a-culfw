
#include <avr/io.h>
#include <util/delay.h>
#include "led.h"


int main(void) {

     DDRA = _BV( PA3 );
     
     
     while (1) {

	  for (uint8_t i=0;i<5;i++)
	       _delay_ms(100);

	  PORTA ^= _BV( PA3 );
	  
	  
     }
     
}
