
#include <avr/io.h>
#include <util/delay.h>
#include "led.h"
#include "fs20.h"
#include "cc1100.h"
#include <avr/interrupt.h>

uint16_t get_adcw(void) {
     uint8_t i;
     uint16_t result = 0;

     CLEAR_BIT( DDRA,  PA1 );
     CLEAR_BIT( PORTA, PA1 );

     // Frequenzvorteiler setzen auf 64 (6) und ADC aktivieren (1)
     
//     ADCSRB = 0;
//     ADCSRA |= _BV(ADEN) | _BV(ADPS1) | _BV(ADPS2); // 125kHz
     ADCSRA |= _BV(ADEN);

//     while ( bit_is_set(ADCSRA, ADSC) ); // wait for converter to finish
//     ADMUX = 0x21;
     
     ADCSRA |= _BV(ADSC);                // Dummy readout
     while ( bit_is_set(ADCSRA, ADSC) ); // wait for converter to finish
     result +=  (ADCL | (ADCH << 8));      // Wandlungsergebnisse aufaddieren
     
     _delay_ms(1);
     
     result = 0;

     /* Eigentliche Messung - Mittelwert aus 2 aufeinanderfolgenden Wandlungen */
     for(i=0; i<2; i++) {
	  ADCSRA |= _BV(ADSC);                // Dummy readout
	  while ( bit_is_set(ADCSRA, ADSC) ); // wait for converter to finish
	  result +=  (ADCL | (ADCH << 8));      // Wandlungsergebnisse aufaddieren
     }

     ADCSRA &= ~_BV(ADEN);                // ADC deaktivieren (2)
     
     result /= 2; 
     
     return result;
}

int main(void) {

//     OSCCAL = 0x8A;

//     while (1);

     ADCSRA = _BV(ADPS1) | _BV(ADPS2); // 125kHz
     ADCSRB = 0;
     ADMUX  = 1 | _BV(REFS1);   // Kanal waehlen + interne Referenzspannung nutzen

     led_init();

     SET_BIT( CC1100_CS_DDR, CC1100_CS_PIN );  // CS as output
     SET_BIT( CC1100_CS_PORT, CC1100_CS_PIN ); // set high
     
     // SPI
     // SCK auf Hi setzen
     // MOSI, SCK, SS als Output
     SPI_DDR  |= _BV( MOSI ) | _BV( SCK ); // mosi, sck output
     // MISO als Input
     SPI_DDR  &= ~_BV( MISO ); // miso input
     
     ccInitChip();

     fs20_init();
     
     sei();

/*
     while ((cc1100_readReg( CC1100_MARCSTATE ) & 0x1f) != 13)
	  ccStrobe( CC1100_SRX );
*/

     // switch on sensor
     SET_BIT( DDRA,  PA2 );
     SET_BIT( PORTA, PA2 );

     // pull switch
     SET_BIT( PORTA, PA0 );
     
     uint8_t i = 0;
     
     while (1) {

	  _delay_ms(100);
	  
	  if (i++==0) {
	       fs20_send_adc( get_adcw() );
	  }
	  
	  if (!bit_is_set( PINA, PA0 )) {
	       LED_ON();
	       while (!bit_is_set( PINA, PA0 ));
	       i = 0;
	  }
	  
	  LED_OFF();
//	  Analyze_Task();

	  
     }
     
	  

}
