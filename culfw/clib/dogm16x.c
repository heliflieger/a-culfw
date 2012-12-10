#include <avr/io.h>
#include <avr/pgmspace.h>

#include "delay.h"
#include "spi.h"
#include "stringfunc.h"
#include "board.h"
#include "dogm16x.h"
#include "../version.h"

static uint8_t cpos = 0;

void dogm_send_ctrl( uint8_t data ) {
     DOGM_RS_PORT &= ~_BV(DOGM_RS_PIN);

     spi_send( data );

     DOGM_RS_PORT |= _BV(DOGM_RS_PIN);	

     my_delay_ms(2);

}

void dogm_put_char( char data ) {
     DOGM_RS_PORT |= _BV(DOGM_RS_PIN);	

     spi_send( data );

     DOGM_RS_PORT &= ~_BV(DOGM_RS_PIN);	

     my_delay_us(50);
}

void dogm_put_string( char *s ) {
     DOGM_CS_PORT &= ~_BV(DOGM_CS_PIN);		

     while(*s)
	  dogm_put_char(*s++);

     DOGM_CS_PORT |= _BV( DOGM_CS_PIN );
}

void dogm_put_string_P(const char *s) {
     uint8_t c;

     DOGM_CS_PORT &= ~_BV(DOGM_CS_PIN);		

     while((c = __LPM(s))) {
	  dogm_put_char(c);
	  s++;
     }

     DOGM_CS_PORT |= _BV( DOGM_CS_PIN );
}

void dogm_control( uint8_t data ) {
     DOGM_CS_PORT &= ~_BV(DOGM_CS_PIN);		
     dogm_send_ctrl( data );
     DOGM_CS_PORT |= _BV( DOGM_CS_PIN );
}

void dogm_clear( void ) {
     dogm_control( 1 );
}

void dogm_set_cursor( uint8_t pos ) {
     dogm_control( 128 + pos );
}

void dogm_contrast( uint8_t con ) {
     DOGM_CS_PORT &= ~_BV(DOGM_CS_PIN);		
     dogm_send_ctrl( 0b00111001 );
     dogm_send_ctrl( 0b01110000 | (con&0xf) );
     dogm_send_ctrl( 0b00111000 );
     DOGM_CS_PORT |= _BV( DOGM_CS_PIN );
}

	
void dogm_init( void ) {

     // set Port pins
     DOGM_RS_DDR  |= _BV( DOGM_RS_PIN );
     DOGM_RS_PORT |= _BV( DOGM_RS_PIN );
     
     DOGM_CS_DDR  |= _BV( DOGM_CS_PIN );

     // initial init
     DOGM_CS_PORT &= ~_BV(DOGM_CS_PIN);		

     dogm_send_ctrl( 0x39 );
     dogm_send_ctrl( 0x15 );
     dogm_send_ctrl( 0x55 );
     dogm_send_ctrl( 0x6e );
     dogm_send_ctrl( 0x71 );
     dogm_send_ctrl( 0x38 );
     dogm_send_ctrl( 0x0f );
     dogm_send_ctrl( 0x06 );
     
     DOGM_CS_PORT |= _BV( DOGM_CS_PIN );

     dogm_clear();

     dogm_put_string_P( PSTR("* "));
     dogm_put_string_P( PSTR(BOARD_ID_STR));
     dogm_put_string_P( PSTR(" V"));
     dogm_put_string_P( PSTR(VERSION));
     dogm_put_string_P( PSTR(" *"));

     dogm_set_cursor( 0x10 );
     dogm_put_string_P( PSTR("   busware.de") );

     dogm_set_cursor( 0x20 );
     dogm_put_string_P( PSTR("booting up ... ") );

     cpos = 0;
}

void dogm_func(char *in) {
     uint8_t xx;
     


     switch (in[1]) {
	  
     case 'c': // display clear
	  dogm_clear();
	  break;
	  
     case 't': // write text: 00TEXT ( 2 hex digts pos + text )
	  fromhex(in+2, &xx, 2);
	  xx = (xx>0x2f) ? 0x2f : xx;
     	  dogm_set_cursor( xx );
	  in[52-xx] = 0;
	  dogm_put_string( in+4 );
	  break;
	  
     case 'C': // native control byte (hex)
	  fromhex(in+2, &xx, 2);
	  dogm_control( xx );
	  break;
	  
     case 'b': // Contrast 00..0f
	  fromhex(in+2, &xx, 2);
	  dogm_contrast( xx );
	  break;
	  
     }
     

}

void dogm_putchar(char c) {

     if (c == '\n') {
	
	  // fill line with spaces ...
	  DOGM_CS_PORT &= ~_BV(DOGM_CS_PIN);		

	  while (cpos++<16)
	       dogm_put_char(' ');

	  DOGM_CS_PORT |= _BV( DOGM_CS_PIN );

	  cpos = 0;

     } else if (c == '\r') {
	  // eat it ...
	  
     } else if( cpos<16 ) {

	  if (cpos==0) 
	       dogm_set_cursor( 0x20 );

	  DOGM_CS_PORT &= ~_BV(DOGM_CS_PIN);		
	  
	  dogm_put_char(c);
	  cpos++;
	  
	  DOGM_CS_PORT |= _BV( DOGM_CS_PIN );
     }
     
}
