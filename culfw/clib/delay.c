#include <avr/io.h>
#include "util/delay_basic.h"
#include "delay.h"



void
my_delay_us( uint16_t d )
{
#if 1
  _delay_loop_2(d<<2);  // Assuming 8MHz & 4cycle per loop2
#else
  TCNT1 = 0;            // The timer must be set up to run with 1MHz
  while (TCNT1<d) {};
#endif
}

void
my_delay_ms( uint16_t d )
{
  for(uint16_t i=0;i<d;i++)
    my_delay_us( 1000 );
}
     

