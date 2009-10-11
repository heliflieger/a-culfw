/*

	bsfw firmware for CPM-BS from busware.de
	written 2009 by Dr. Boris Neubert  omega at online dot de

	device description: http://www.busware.de/tiki-index.php?page=CPM-BS


	After powerup, the device reads its sensor number from the eeprom.
	Possible sensor numbers are 1 to 9.

	The first ten transmissions (prelude mode) come every 8 seconds, then
	the transmission interval changes to 2 minutes (normal operation).

	To enter configuration mode hold down the button on the device until
	the LED comes on - this should take at most 8 seconds. Release the
	button then. The LED will indicate the sensor number (blink once=
	sensor number 1, blink twice= sensor number 2, etc.). There is a one
	second pause between subsequent blink sequences. If you press and
	release the button in the pause the sensor number is increased by one.
	The sensor number wraps around from 9 to 1. If you do not press the
	button for ten pauses, the device re-enters prelude mode followed by
	normal operation after ten transmissions.

*/


#include <avr/io.h>
#include <util/delay.h>
#include "board.h"
#include "fs20.h"
#include "cc1100.h"
#include "config.h"
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/wdt.h>



uint8_t sensor_number;	// 1..9



// initialize the brightness sensor
void init_bs(void) {

	led_init();

	SET_BIT( CC1100_CS_DDR, CC1100_CS_PIN );  // CS as output
	SET_BIT( CC1100_CS_PORT, CC1100_CS_PIN ); // set high

	// SPI
	// set SCK to high, use MOSI, SCK, SS as output
     	SPI_DDR  |= _BV(MOSI) | _BV(SCK);
     	// MISO as input
     	SPI_DDR  &= ~_BV(MISO);

     	fs20_init();

     	sei();

     	// switch on sensor
     	SET_BIT( DDRA,  PA2 );
     	SET_BIT( PORTA, PA2 );

     	// pull switch
     	SET_BIT( PORTA, PA0 );

}


// read sensor # from eeprom
void load_sensor_number(void) {
	sensor_number= get_config_byte(CFG_OFS_SENSOR);
	if((sensor_number<1)||(sensor_number>9)) {
		sensor_number= 1;
	}
}

// write sensor # to eeprom
void save_sensor_number(void) {
	set_config_byte(CFG_OFS_SENSOR, sensor_number);
}


// make LED blink n times
void blink(uint8_t n) {

	for(uint8_t i= 0; i< n; i++) {
		LED_ON();
		_delay_ms(150);
		LED_OFF();
		_delay_ms(250);
	}
}

// check for button down
uint8_t button_down(void) {
	return !bit_is_set( PINA, PA0 );
}


// sensor number selection
void configure_sensor_number(void) {

	#define IDLE 10

	uint8_t idle= 0;

	while(idle<IDLE) {
		// blink sensor # times
		blink(sensor_number);
		// poll for key press (maximum 1 second)
		for(uint8_t i= 10; i>0; i--) {
			if(button_down()) {
				sensor_number++;
				if(sensor_number> 9) {
					sensor_number= 1;
				}
				while(button_down());
				save_sensor_number();
				idle= 0;
				break;
			}
			_delay_ms(100);
		}
		idle++;
	}
}

// get voltage: bs output voltage/reference voltage*1023, 10bit resolution
uint16_t get_adcw(void) {

     	uint8_t lo, hi;
     	uint16_t result = 0;

     	CLEAR_BIT( DDRA,  PA1 );
     	CLEAR_BIT( PORTA, PA1 );

	// disable ADC
     	ADCSRA &= ~_BV(ADEN);

     	// internal voltage reference, single ended input PA1, 1.1V
     	ADMUX= _BV(REFS1) | 1;


	// right adjusted 10bit result in ADCH/ADCL
	ADCSRB &= ~_BV(ADLAR);

	// Auto Trigger Disable
	ADCSRA &= ~_BV(ADATE);

	// Interrupt Disable
	ADCSRA &= ~_BV(ADIE);

	// maximum resolution if clock frequencz between 50kHz and 200kHz
	// prescaler division factor= 64 gives 8MHz/64= 125kHz
	ADCSRA |= _BV(ADPS2) | _BV(ADPS1);
	ADCSRA &= ~_BV(ADPS0);

	// enable ADC
	ADCSRA |= _BV(ADEN);

	// start conversion
	ADCSRA |= _BV(ADSC);

	// wait for ADC to finish
	while ( bit_is_set(ADCSRA, ADSC) ); // wait for converter to finish

     	// ADCL must be read first, to ensure contents belong to same conversion
     	lo= ADCL;
     	hi= ADCH;
     	result= hi << 8 | lo;

	// disable ADC
     	ADCSRA &= ~_BV(ADEN);

	// 10bit right-adjusted
     	return result;

     	// note: the light sensor's output voltage Vout is independent of Vcc
     	// Vout= 1.8muA*sqrt(E/100lux)*RExt
     	// => E= 100lux*(Vout/RExt/1.8muA)^2
     	// with Vout= 1.1V*result

}

// build and send FS20 datagram
void send_data(void) {

	// power up CC1101 and initialize
	ccInitChip();

	uint8_t data[5];

	uint16_t adcw= get_adcw() & 0x3ff;// 10 bit reading

 	uint8_t sensor= sensor_number;	// sensor # (1..9)
	uint8_t transmit_flags = 0x00;		// transmit_flags (none)

	data[0]= 0xa5; data[1]= 0xcf;	// house code a5cf
	data[2]= sensor;
	// fff1ffdd dddddddd
	data[3]= transmit_flags | 0x20 | (adcw >> 8);
	data[4]= (adcw & 0xff);

	// send datagram
	fs20_send(data);
}

// enable watchdog timer interrupt
void enable_wdi(void) {
	// see p44 in ATtiny84 manual

	// disable watchdog
	MCUSR &= ~_BV(WDRF);
	WDTCSR |= _BV(WDCE) | _BV(WDE); WDTCSR &= ~_BV(WDE);
	// 8.0 sec
	WDTCSR |= _BV(WDP3) | _BV(WDP0); WDTCSR &= ~ ( _BV(WDP2) | _BV(WDP1) );
	// 4.0 sec
	//WDTCSR |= _BV(WDP3); WDTCSR &= ~ ( _BV(WDP2) | _BV(WDP1) | _BV(WDP0) );
	// 1.0 sec
	//WDTCSR |= _BV(WDP2) | _BV(WDP1); WDTCSR &= ~ ( _BV(WDP3) | _BV(WDP0) );
	// enable watchdog timeout interrupt
	sei();
	WDTCSR |= _BV(WDIE);
}


// transmission interval: 15*8sec= 2min
#define SKIP 15
uint8_t skip= SKIP;

// number of initial transmission with 8 sec spacing
#define PRELUDE 10
uint8_t prelude= PRELUDE;

// transmit_flag signals that brightness should be transmitted
uint8_t transmit_flag= 1;

// button_flag signals that button was pressed
uint8_t button_flag= 0;

// the watchdog takes us here every 8 seconds
ISR(WDT_vect) {

	//MCUSR &= ~(1 << WDRF);

	button_flag= button_down();
	if(!skip) {
		// every SKIPth time transmit_flag is set
		transmit_flag= 1;
		skip= SKIP;
	} else {
		if(prelude) {
			prelude--;
			transmit_flag= 1; }
		else {
			transmit_flag= 0;
		}
		skip--;
	}
}


// power down CC1101 and ATtiny84
void power_down(void) {

		// turn CC1101 into power down mode
		ccStrobe(CC1100_SPWD);

		// disable ADC if not already disabled
     		ADCSRA &= ~_BV(ADEN);

     		// disable AC
     		ACSR |= _BV(ACD);

	     	// power-down mode (10)
     		MCUCR |= _BV(SM1); MCUCR &= ~_BV(SM0);

     		// sleep enable
     		MCUCR |= _BV(SE);

		/*
		// disable BOD (not on ATtiny84)
		MCUCR |= _BV(BODS) | _BV(BODSE);
		MCUCR |= _BV(BODS); MCUCR &= ~_BV(BODSE);*/

		// enter sleep mode
		sleep_mode();

}

//
// main function of bs firmware
//
int main(void) {

	// init hardware
        init_bs();

        // get sensor #
        load_sensor_number();

	// enable watchdog
	enable_wdi();

	// loop forever
	prelude= PRELUDE;
        while(1) {
        	if(button_flag) {
        		// enter configuration dialog
        		LED_ON();
        		while(button_down());
        		LED_OFF();
			configure_sensor_number();
			prelude= PRELUDE;
		};
        	// transmit_flag is set at startup and every 2 minutes
        	if(transmit_flag) {
        		// send FS20 datagram
        		send_data();
        		// allow some time for transfer to finish
			_delay_ms(250);
		}
		// go to sleep for 8 seconds
		power_down();
	}

}
