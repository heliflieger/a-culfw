#include <avr/io.h>
#include "delay.h"

// The timer must be set up to run with 1MHz

void my_delay_us( uint16_t d ) {
     TCNT1 = 0;
     while (TCNT1<d) {};
}

void my_delay_ms( uint16_t d ) {
     for (uint16_t i=0;i<d;i++)
	  my_delay_us( 1000 );
}
     

