#include "actions.h"
#include "board.h"
#include "util.h"
#include "config.h"
#include "cc1100.h"
#include "fs20.h"
#include <util/delay.h>
#include <avr/eeprom.h>
#include "math.h"

#define FS20_CODE_ON	0x11
#define FS20_CODE_OFF	0x00

uint16_t measure(void) {

	uint8_t lo, hi;
  	// start conversion
	ADCSRA |= _BV(ADSC);
	// wait for ADC to finish
	while ( bit_is_set(ADCSRA, ADSC) ); // wait for converter to finish
     	// ADCL must be read first, to ensure contents belong to same conversion
     	lo= ADCL;
     	hi= ADCH;
     	return (hi << 8 | lo);
}


uint16_t get_voltage(void) {

	uint16_t result= 0;
	
     	CLEAR_BIT( DDRA,  PA0 );
     	CLEAR_BIT( PORTA, PA0 );
     	CLEAR_BIT( DDRA,  PA2 );
     	CLEAR_BIT( PORTA, PA2 );

	// disable ADC
     	ADCSRA &= ~_BV(ADEN);

     	// External voltage reference at PA0 (AREF) pin. See ATTiny84 manual p. 145
	// measure ADC2
     	ADMUX |= _BV(REFS0) | _BV(MUX1); 
	ADMUX &= ~_BV(REFS1) & ~_BV(MUX0) & ~_BV(MUX2) & ~_BV(MUX3) & ~_BV(MUX4) & ~_BV(MUX5);
	
	// Vcc as ref
     	//ADMUX |= _BV(MUX1); 
	//ADMUX &= ~_BV(REFS1) & ~_BV(REFS0) & ~_BV(MUX0) & ~_BV(MUX2) & ~_BV(MUX3) & ~_BV(MUX4) & ~_BV(MUX5);


	// 1.1V as ref
	//ADMUX |= _BV(REFS1) | _BV(MUX1); 
	//ADMUX &= ~_BV(REFS0) & ~_BV(MUX0) & ~_BV(MUX2) & ~_BV(MUX3) & ~_BV(MUX4) & ~_BV(MUX5);

	// right adjusted 10bit result in ADCH/ADCL
	ADCSRB &= ~_BV(ADLAR);

	// Auto Trigger Disable
	ADCSRA &= ~_BV(ADATE);

	// Interrupt Disable
	ADCSRA &= ~_BV(ADIE);

	// Internal RC Oscillator runs at 8 MHz.
	// maximum resolution if clock frequencz between 50kHz and 200kHz
	// prescaler division factor= 64 gives 8MHz/64= 125kHz
	ADCSRA |= _BV(ADPS2) | _BV(ADPS1);
	ADCSRA &= ~_BV(ADPS0);

	// enable ADC
	ADCSRA |= _BV(ADEN);
	
	result= measure(); // throw away
	result= (measure()+measure()+measure()+measure()) >> 2;

	// disable ADC
     	ADCSRA &= ~_BV(ADEN);

	// 10bit right-adjusted
     	return result;

	
}

 

/*
 * 
 */
void fs20_sendsensor(void) {
  
  
  /*
   * MPX5010 Transfer Function
   * Vout= Vs x f(P) or f(P)= Vout/Vs
   * with f(P)= (0.09/kPa x P + 0.04)       +-5,0%Vfss (0..85 degrees Celsius)
   * P= (f(P) - 0.04) / 0.09/kPa
   */
  // f(P)
  float fP= get_voltage()/1024.0;
  float P=  (fP - (FOFFSET)-(CORR1))/(FSCALE); // pressure in kPa
  if(P< 0.0) { P= 0.0; }
  
  #ifdef SEND_PRESSURE
  // P= 0..10 
  uint8_t result= round(P*10.0); // hPa 0..100
  #else
  uint8_t result= (USFOFFSET)+(USFHEIGHT)-round(P*9.8066*(CORR2));
  #endif

  // 
  // send command
  fs20_sendValue(HOUSECODE, BUTTON, 0, result);
  // fiddling with the LED breaks the transfer in cases
  // when the LED is connected to PA2! 
  _delay_ms(250); // wait some time for transfer to finish
  // a powerdown might break the transfer!

}


/*
 * 
 */
void action_getconfig(void) {
  // intentionally left blank
}

void action_keypressshort(void) {
  /*for(uint8_t i= 0; i< 10; i++) {
    fs20_sendsensor();
    _delay_ms(1000);
  }
  */
  fs20_sendsensor();
  reset_clock();
}

void action_keypresslong(void) {
  // intentionally left blank
}

void action_timeout(void) {
  fs20_sendsensor();
}

void action_pinchanged(void) {
  // intentionally left blank
}