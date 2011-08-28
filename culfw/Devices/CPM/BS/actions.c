#include "actions.h"
#include "board.h"
#include "util.h"
#include "config.h"
#include "cc1100.h"
#include "fs20.h"
#include <util/delay.h>
#include <avr/eeprom.h>
#include "math.h"

uint8_t sensor_number;  // 1..9


uint8_t	cfgSensor	EEMEM;

void load_sensor_number(void) {
        sensor_number= get_config_byte(&cfgSensor);
        if((sensor_number<1)||(sensor_number>9)) {
                sensor_number= 1;
        }
}

// write sensor # to eeprom
void save_sensor_number(void) {
        set_config_byte(&cfgSensor, sensor_number);
}

// sensor number selection
void configure_sensor_number(void) {

        #define IDLE 10

        uint8_t idle= 0;

        while(idle<IDLE) {
                // blink sensor # times
                led_blink(sensor_number);
                // poll for key press (maximum 1 second)
                for(uint8_t i= 10; i>0; i--) {
                        if(SW_STATE()) {
                                sensor_number++;
                                if(sensor_number> 9) {
                                        sensor_number= 1;
                                }
                                while(SW_STATE());
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

        uint8_t sensor= sensor_number;  // sensor # (1..9)
        uint8_t transmit_flags = 0x00;          // transmit_flags (none)

        data[0]= 0xa5; data[1]= 0xcf;   // house code a5cf
        data[2]= sensor;
        // fff1ffdd dddddddd
        data[3]= transmit_flags | 0x20 | (adcw >> 8);
        data[4]= (adcw & 0xff);

        // send datagram
        fs20_send(data, 5);

	// wait for datagram to be sent
	_delay_ms(250);
}




/*
 * 
 */
void action_getconfig(void) {
  load_sensor_number();
}

void action_keypressshort(void) {
  /*for(uint8_t i= 0; i< 10; i++) {
    fs20_sendsensor();
    _delay_ms(1000);
  }
  */
  send_data();
  reset_clock();
}

void action_keypresslong(void) {
  configure_sensor_number();
}

void action_timeout(void) {
  send_data();
}

void action_pinchanged(void) {
  // intentionally left blank
}