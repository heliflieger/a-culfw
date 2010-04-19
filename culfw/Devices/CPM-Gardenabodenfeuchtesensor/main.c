/*

	Firmware for the Gardena Bodenfeuchtesensor FS20 Modul
	Further information upon schematics and software is available at:
	http://knowhow.amazers.net/space/dev/projects/hardware/GardenaBodenfeuchteSensorFS20Modul

	(c) 2010 by Maz Rashid - maz @ amazers . net
	Under GPL License. This work is based on software provided by Dirk Tostmann, busware.de


	Short description:
	The device measures the Gardena soil humidity sensor value and sends FS20-Telegrams.
	Upon this you could manage to automatically water your garden.
	It will send a FS20-On-Command if it is dry and will send an OFF-Command if it is humid.

	Setup:
	Pressing the button for a period longer than 1sec will set the device in the program mode.
	(shown by the led light). The device will wait for an incoming FS20-telegram and
	take over it's housecode and buttoncode.

	A short press on the button will force the device to immediately send the current sensor value.
	The sensor value will be sent every time it changes. It will be also sent, if within the last
	hour there were no telegrams to send. (No change, No button).

	The device will remain most of the time in the Powerdown-Sleep mode to decrease the power consumption.
	The Watchdog-Interrupt will wake it every 8s.

	After powerup, the device reads its sensor number from the eeprom.
	Possible sensor numbers are 1 to 9.
*/

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <avr/eeprom.h>
#include "board.h"
#include "config.h"
#include "util.h"
#include "fs20.h"
#include "cc1100.h"

#define FS20_CODE_ON	0x11
#define FS20_CODE_OFF	0x00

// The watchdog timer interrup occurs every 8s. 1hr are 450x8s.
// measurements resultest that the watchdog needs around 9s
#define LONGTIMER_VAL 3600/9

volatile uint8_t  oldSensorValue = 0;
volatile uint16_t longTimerCnt = 0;

void init(void)
{
	LED_INIT();
	SW_INIT();
	SENS_INIT();

    // SPI
    // MOSI, SCK, als Output / MISO als Input
    SPI_DDR  |= _BV( SPI_MOSI ) | _BV( SPI_SCLK ); // mosi, sck output
    SPI_DDR  &= ~_BV( SPI_MISO ); // miso input

    // test pin change interrupt
	GIMSK |= _BV(PCIE0); 	/* PCIE0 pin change interrupt auf pcint 7 bis 0 aktiviert */
	PCMSK0 |= _BV(PCINT0) | _BV(PCINT1);  /* pcint0 + 1 aktivieren */

	// read housecode/button from eprom
	readConfig();

    // init devices
    ccInitChip();
    fs20_init();

    // init watchdog timer as long timer (8s). Timeout will lead to an interrupt and not a reset
    // do not set WDE (as if so we need to set WDIE after every interrupt)
    WDTCSR |= _BV(WDIE) | _BV(WDP3) | _BV(WDP0);

}


void fs20_sendSensorValue(uint8_t sensorStatus)
{
	uint8_t fs20_cmd = sensorStatus?FS20_CODE_ON:FS20_CODE_OFF;

	uint8_t data[4];
	data[0] = fs20_housecode>>8;
	data[1] = fs20_housecode & 0xff;
	data[2] = fs20_button;
	data[3] = fs20_cmd;

	fs20_send(data,4);
}

void handleSensor(uint8_t force)
{
	// activate sensor, get value, deactivate sensor
	SENS_ACTIVATE(1);
	_delay_us(100);
	uint8_t val = SENS_IS_DRY();
	SENS_ACTIVATE(0);

	// send sensor value, if in force mode or if a change happened
	if(force || oldSensorValue!=val)
	{
		fs20_sendSensorValue(val);
		oldSensorValue = val;
		longTimerCnt = 0;
		led_blink(1);
	}
}

int main(void)
{
	SetSystemClockPrescaler(0);
	init();
	sei();

	led_blink(3);

     while (1)
     {
    	if(SW_ACTIVE())
    	{
    		// SW pressed
    		uint8_t pressCnt = 100;

    		// wait until released
    		do
    		{
				// long term press detected?
    			if(pressCnt>0 && --pressCnt==0)
    				LED_ON();

    			_delay_ms(10);
    		} while(SW_ACTIVE());


    		// SW released
    		LED_OFF();
    		if(pressCnt>0)
    			handleSensor(1); // force transmit
    		else
    		{
    			// program mode
        		LED_ON();
        		fs20_resetbuffer();
        		fs20_rxon();
        		fstelegram_t t;
        		t.type = 'U'; // undefined
        		pressCnt = 100;

        		// wait for next telegram
        		// exit if button pressed or timeout occured
        		while(!fs20_readFS20Telegram(&t) && --pressCnt>0 && !SW_ACTIVE())
        			_delay_ms(100);

        		fs20_idle();
        		LED_OFF();

        		// save the result
        		if(t.type=='F')
        		{
        			saveConfig(t.housecode, t.button);
        			led_blink(2); // confirm save of config
        			handleSensor(1); // force transmit
        		}

        		// wait until sw releases
        		while(SW_ACTIVE());
    		}
    	}

		// check long term timer
		if(longTimerCnt >= LONGTIMER_VAL)
			handleSensor(1);

		// finaly check if sensor value changed
		// this will only be the case if the current sensor value haven't been sent
		// during this cycle
		handleSensor(0);

		// sleep well
		LED_OFF();
		SENS_ACTIVATE(0);
		set_sleep_mode(SLEEP_MODE_PWR_DOWN);
		sleep_mode();
     }
}


ISR(PCINT0_vect)
{
	// This interrupt is only used to wake up the microcontroller on level changes of the input pins
	// No action needed here.
}

ISR(WATCHDOG_vect)
{
	// This interrupt service routine is called every 8s
	// the long time counter will ensure that the sensor value is sent at least once an hour.
	//LED_TOG();
	// every 8s count 1 tick up.
	longTimerCnt++;
}
