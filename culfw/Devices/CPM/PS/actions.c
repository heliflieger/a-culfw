#include "actions.h"
#include "board.h"
#include "util.h"
#include "config.h"
#include "cc1100.h"
#include "fs20.h"
#include <util/delay.h>
#include <avr/eeprom.h>
#include "math.h"

// magic word
#define MAGIC			0x2011

// fix house code
#define HOUSECODE 		(0xa5ce)

// for USF1000 emulation
#define BUTTON_DISTANCE 	(0xaa)

// for PS
#define BUTTON_HEIGHT		(0xab)

// for commands
#define BUTTON_COMMAND		(0xac)

// #define COMMAND_SENDSTATUS 	(0x17)

// extension bit set (bit 5, 0x20)
#define COMMAND_USFOFFSET	(0x3c)
#define COMMAND_USFHEIGHT	(0x3d)
#define COMMAND_HCALIB1		(0x3e)
#define COMMAND_HCALIB2		(0x3f)

// pressure sensor calibration default values
#define HSCALE_DEFAULT (1.133)
#define HOFFSET_DEFAULT (0.04532)

// USF1000 emulation (in meters) default values
#define USFOFFSET_DEFAULT (0.50)
#define USFHEIGHT_DEFAULT (1.00)

// EEPROM variables
uint16_t	cfgMagic	EEMEM;
float		cfgHScale	EEMEM;
float		cfgHOffset	EEMEM;
float		cfgUSFOffset	EEMEM;
float		cfgUSFHeight	EEMEM;
uint8_t		cfgBlinker	EEMEM;


// USF1000 emulation
float usfoffset;
float usfheight;

// the formula to calculate the height is
//	h= hscale*x + hoffset
float hscale;
float hoffset;

// this is for calibration
float hcalib1= 0.0; float xcalib1= 0.04;
float hcalib2= 1.0; float xcalib2= 0.94;

/*
 * save hscale and hoffset to EEPROM
 */
void set_hconfig(float scale, float offset) {
  hscale= scale;
  hoffset= offset;
  set_config_float(&cfgHScale, hscale);
  set_config_float(&cfgHOffset, hoffset);
}

/*
 * save usfoffset and usfheight
 */

void set_usfconfig(float offset, float height) {
  usfoffset= offset;
  usfheight= height;
  set_config_float(&cfgUSFOffset, usfoffset);
  set_config_float(&cfgUSFHeight, usfheight);
}
  
/*
 * retrieve hscale,hoffset,usfoffset and usfheight from  EEPROM
 */
void get_config(void) {
  if(get_config_word(&cfgMagic)==MAGIC) {
    hscale= get_config_float(&cfgHScale);
    hoffset= get_config_float(&cfgHOffset);
    usfoffset= get_config_float(&cfgUSFOffset);
    usfheight= get_config_float(&cfgUSFHeight);
  } else {
    set_hconfig(HSCALE_DEFAULT, HOFFSET_DEFAULT);
    set_usfconfig(USFOFFSET_DEFAULT, USFHEIGHT_DEFAULT);
    set_config_word(&cfgMagic, MAGIC);
  }
}

/*
 * retrieve result from ADC once (result is 1024*Vout/Vs)
 */
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

/*
 * measure voltage (result is x= Vout/Vs)
 */
float get_voltage(void) {

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
  
  // two alternatives follow:
  
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
  // average over 4 values
  result= (measure()+measure()+measure()+measure()) >> 2;

  // disable ADC
  ADCSRA &= ~_BV(ADEN);

  // Vout/Vs as 10bit right-adjusted integer
  return result/1024.0;
}

/*
 * get height of water over ground in meters
 */
float get_height(void) {
  // for details see documentation
  float h= hscale*get_voltage()+hoffset;
  if(h<0.0) h= 0.0;
  return h;
}

/*
 * get simulated distance of ultrasound sensor in meters for
 * USF1000 compatibility 
 */
float get_distance(float height) {
  return usfoffset+usfheight-height;
}

/*
 * send sensor data
 */
void fs20_sendsensordata(void) {
  float height= get_height();
  float distance= get_distance(height);
  // send in centimeters
  fs20_sendValue(HOUSECODE, BUTTON_HEIGHT, 0, round(height*100.0));
  while(fs20_busy());
  fs20_sendValue(HOUSECODE, BUTTON_DISTANCE, 0, round(distance*100.0));
  while(fs20_busy());
  // wait for transfer to finish since a powerdown might break the transfer!
}

/*
 * recalibrate
 */
void recalibrate(void) {
  float scale= (hcalib2-hcalib1)/(xcalib2-xcalib1);
  float offset= hcalib1-scale*xcalib1;
  set_hconfig(scale, offset);
}
 
/*
 * remote configuration facility
 */
void fs20_configuration(void) {
  
  ccInitChip();
  
  fs20_resetbuffer();
  fs20_rxon();

  fstelegram_t t;
  t.type = 'U'; // undefined
  uint16_t rxtimeout = 300; // wait 300*100ms= 30s for reception

  // wait for next telegram
  // exit if button pressed or timeout occured
  while(!fs20_readFS20Telegram(&t) && (rxtimeout--)>0) {
    _delay_ms(100);
    if(LED_PIN!=PA3) LED_TOGGLE(); // do not touch pin for CC1100 communication
  }
  fs20_idle();

  // evaluate the result
  if(t.type=='F')   {
    if((t.housecode==HOUSECODE) && (t.button==BUTTON_COMMAND)) {
      led_blink(3); // blink for confirmation
      switch(t.cmd) {
	case COMMAND_HCALIB1:
	  xcalib1= get_voltage();
	  hcalib1= t.data[0]/100.0;
	  recalibrate();
	  break;
	case COMMAND_HCALIB2:
	  xcalib2= get_voltage();
	  hcalib2= t.data[0]/100.0;
	  recalibrate();
	  break;
	case COMMAND_USFOFFSET:
	  // we receive ufsoffset in centimeters
	  set_usfconfig(t.data[0]/100.0, usfheight);
	  break;
	case COMMAND_USFHEIGHT:
	  // we receive usfheight in centimenters
	  set_usfconfig(usfoffset, t.data[0]/100.0);
	  break;
      }
    }
  }   
}

/*
 * 
 */
void action_getconfig(void) {
  get_config();
}

void action_keypressshort(void) {
  fs20_sendsensordata();
  reset_clock();
}

void action_keypresslong(void) {
  fs20_configuration();
}

void action_timeout(void) {
  fs20_sendsensordata();
}

void action_pinchanged(void) {
  // intentionally left blank
}