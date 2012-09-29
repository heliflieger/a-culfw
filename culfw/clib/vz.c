#include "board.h"

#ifdef HAS_VZ

#include <avr/interrupt.h>
#include <avr/io.h>
    
#include "led.h"
#include "serial.h"
#include "vz.h"
#include <string.h>

#include "ttydata.h"
#include "display.h"

volatile uint8_t vz_temp[VZ_MSG_SIZE];
volatile uint8_t vz_data[VZ_MSG_SIZE];
volatile uint8_t vz_state = 0;
volatile uint16_t vz_pos = 0;
uint16_t  vz_auto = 0;
uint16_t  vz_next = 0;

// TX complete (data buffer empty) 
/*
ISR(USART1_UDRE_vect)
{
     if (TTY_Tx_Buffer.nbytes) {
	  
	  UDR0 = rb_get(&TTY_Tx_Buffer);
	  
     } else {
	  
	  UCSR0B &= ~_BV(UDRIE0);
	  
     }
     
}
*/

// RX complete
ISR(USART1_RX_vect)
{

     LED_TOGGLE();

     /* read UART status register and UART data register */ 
     uint8_t data = UDR1;
     uint8_t usr  = UCSR1A;
     
     if ((usr & (_BV(FE1)|_BV(DOR1))) == 0) {

	switch (vz_state) {
	  case 0:
	    if (data == '/') { // start of telegram
	      vz_state   = 1;
              vz_pos     = 0;
	    } 
	    break;
	  case 1:
	    if (data == '!') { // end of telegram
	      memcpy( (uint8_t *)vz_data, (uint8_t *)vz_temp, vz_pos );
	      vz_data[vz_pos] = 0;
	      vz_state = 0;
	    } else {
              vz_temp[vz_pos++] = data;
	      if (vz_pos>=VZ_MSG_SIZE)
		vz_state = 0;
	    }
	    break;
	}

     } else {
       // receive ERROR - reset reception
       vz_state = 0;
     }

     
}

void vz_init(void) {
	
     unsigned int baudrate = UART_BAUD_SELECT_DOUBLE_SPEED(9600,F_CPU);

     /* Set baud rate */
     if ( baudrate & 0x8000 ) 
     {
	  UCSR1A = (1<<U2X1);  //Enable 2x speed 
	  baudrate &= ~0x8000;
     }

     UBRR1 = baudrate;
     
     /* Enable USART receiver and transmitter and receive complete interrupt */
     UCSR1B = _BV(RXCIE1)|(1<<RXEN1); // |(1<<TXEN1);
    
     /* Set frame format: asynchronous, 8data, no parity, 1stop bit */   
     //UCSR1C = (1<<UCSZ10)|(1<<UCSZ11);
     
     /* Set frame format: asynchronous, 7data, even parity, 1stop bit */   
     UCSR1C = (1<<UPM11)|(1<<UCSZ11);
     
     vz_state   = 0;
     vz_data[0] = 0;
}

void vz_print(void) {

	uint16_t pos = 0;
	uint8_t data;

	if (! vz_data[pos])
	  return;

	DC( 'o' );

	while (vz_data[pos]) {
	  data = vz_data[pos++];
	  switch (data) {
	    case 10:
	      break;
            case 13:
              data = '|';
	    default:
      	      DC(data);
	  }
	}
	DNL();

}

void vz_task(void) {
//     if (!bit_is_set(UCSR1B, UDRIE1) && TTY_Tx_Buffer.nbytes)
//	  UCSR1B |= _BV(UDRIE1);
}

void vz_sectask(void) {

	if (vz_auto) {
	  if (--vz_next == 0) {
	  	vz_print();
		vz_next = vz_auto;
	  }
	}

}

void vz_func(char *in) {

  if(in[1] == 'r') {                // print latest record
    vz_print();	

  } else if(in[1] == 'a') {         // automatic reporting

    if (*(in+2)) { 
      fromdec (in+2, (uint8_t *)&vz_auto);
      vz_next = vz_auto;
    } else {
      DU(vz_auto,0); DNL();
    }

  }
}

#endif
