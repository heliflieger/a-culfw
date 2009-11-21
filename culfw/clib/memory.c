#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <avr/io.h>
#include "display.h"

extern char * const __brkval;

uint16_t freeMem(void) {
     char *brkval;
     char *cp;
     
     brkval = __brkval;
     if (brkval == 0) {
	  brkval = __malloc_heap_start;
     }
     cp = __malloc_heap_end;
     if (cp == 0) {
	  cp = ((char *)AVR_STACK_POINTER_REG) - __malloc_margin;
     }
     if (cp <= brkval) return 0;
     
     return cp - brkval;
}


void getfreemem(char *unused) {
     DC('B'); DU((uint16_t)__brkval,           5); DNL();
     DC('S'); DU((uint16_t)__malloc_heap_start,5); DNL();
     DC('E'); DU((uint16_t)__malloc_heap_end,  5); DNL();
     DC('F'); DU((uint16_t)freeMem(),          5); DNL();
}

void testmem(char *unused) {
     char *buf;
     uint16_t size;

     DS_P( PSTR("testing ") );

     size = 32765;
     display_udec(size, 5,' ');

     DS_P( PSTR(" bytes of RAM - ") );
     
     buf = malloc( size );
     
     if (buf != NULL) {

	  // write
          memset( buf, 0x77, size );
	  for (uint16_t i = 0; i<size; i++)
	       *(buf+i) = (i & 0xff);
	  
	  // read
	  for (uint16_t i = 0; i<size; i++) {
	       if (*(buf+i) != (i & 0xff)) {
		    DS_P( PSTR("Problmes at ") );
		    display_udec(i, 5,' ');
		    DNL();
		    free( buf );
		    return;
	       }
	       
	  }
	  
	  free( buf );
	  DS_P( PSTR("OK") );
     } else {

	  DS_P( PSTR("Alloc failed") );
     }
     
     DNL();
}


