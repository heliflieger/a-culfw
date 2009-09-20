
#include <avr/io.h>
#include <util/delay.h>
#include "board.h"
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
     for(i=0; i<5; i++) {
	  ADCSRA |= _BV(ADSC);                // Dummy readout
	  while ( bit_is_set(ADCSRA, ADSC) ); // wait for converter to finish
	  result +=  (ADCL | (ADCH << 8));      // Wandlungsergebnisse aufaddieren
     }

     ADCSRA &= ~_BV(ADEN);                // ADC deaktivieren (2)

     result /= 5;

     return result;
}

void send_data(void) {

	uint8_t data[5];
	uint16_t adcw= get_adcw() & 0x3ff;// 10 bits

	uint8_t sensor= 0x1;		// sensor # (always 1)
	uint8_t flags = 0x00;		// flags (none)

	data[0]= 0xa5; data[1]= 0xcf;	// house code a5cf
	data[2]= sensor;
	// fff1ffdd dddddddd
	data[3]= flags | 0x20 | (adcw >> 8);
	data[4]= (adcw & 0xff);

	LED_ON();
	_delay_ms(250);
	LED_OFF();
	fs20_send(data);
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

     #define INTERVAL 3

     uint8_t i = INTERVAL*10;

     while (1) {

	  _delay_ms(100);

	  if (!i--) {
	       send_data();
	       i = INTERVAL*10;
	  }

	  if (!bit_is_set( PINA, PA0 )) {
	       LED_ON();
	       while (!bit_is_set( PINA, PA0 ));
	       i = INTERVAL*10;;
	       LED_OFF();
	  }

//	  Analyze_Task();


     }



}
