#include "delay.h"

#include <stdint.h>                     // for uint16_t, uint8_t

#include "util/delay_basic.h"           // for _delay_loop_2



void
my_delay_us( uint16_t d )
{
#if 1
  d<<=1;                // 4 cycles/loop, we are running 8MHz
  _delay_loop_2(d);    
#else
  TIMSK1 = 0;           // No interrupt if counter is reached
  TCNT1 = 0;            // The timer must be set up to run with 1MHz
  while (TCNT1<d) {};
#endif
}

void
my_delay_ms( uint8_t d )
{
  while(d--) {
    my_delay_us( 1000 );
  }
}
