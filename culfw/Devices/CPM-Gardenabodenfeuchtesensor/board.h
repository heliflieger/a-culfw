/*
 * board.h
 *
 *  Created on: 10.04.2010
 *      Author: maz
 */

#ifndef _BOARD_H_
#define _BOARD_H_

/*
 * LED
 */
#define LED_PIN PA2
#define LED_PORT PORTA
#define LED_INIT() DDRA|=_BV(LED_PIN)

#define LED_ON() 	LED_PORT|=_BV(LED_PIN)
#define LED_OFF() LED_PORT&=~_BV(LED_PIN)
#define LED_TOG() LED_PORT^=_BV(LED_PIN)

/*
 * Switch
 */
#define SW_PIN 	PA0
#define SW_PORT	PINA
// SW is input, activate internal pullup resistor
#define SW_INIT()	DDRA&=~_BV(SW_PIN);SW_PORT|=_BV(SW_PIN);
#define SW_ACTIVE() ((SW_PORT & _BV(SW_PIN))==0)

/*
 * Sensor
 */
// The sensor will be activated by setting PB0 to hight
// and should be deactivated after measurement
#define SENS_ACTIVATE_PIN PB0
#define SENS_ACTIVATE_PORT PORTB

#define SENS_PIN	PA1
#define SENS_PORT	PINA
// SENS is input, no pullup needed, SENS_Activate is output
#define SENS_INIT()	DDRA&=~_BV(SENS_PIN);DDRB|=_BV(SENS_ACTIVATE_PIN);

#define SENS_ACTIVATE(on) if(on) SENS_ACTIVATE_PORT|=_BV(SENS_ACTIVATE_PIN); else SENS_ACTIVATE_PORT&=~_BV(SENS_ACTIVATE_PIN);

// sensor will be report 1 when it is dry and
// water is needed
#define SENS_IS_DRY()	((SENS_PORT & _BV(SENS_PIN))>0)


/*
 * CC1100 - radio chip
 */
#define CC1100_CS_DDR		DDRA
#define CC1100_CS_PORT		PORTA
#define CC1100_CS_PIN		PA3
#define CC1100_IN_DDR		DDRB
#define CC1100_IN_PORT		PINB
#define CC1100_IN_PIN		PB2
#define CC1100_OUT_DDR		DDRA
#define CC1100_OUT_PORT		PORTA
#define CC1100_OUT_PIN		PA7
#define CC1100_INT			INT0
#define CC1100_INTVECT		INT0_vect

#define SPI_PORT		PORTA
#define SPI_DDR			DDRA
#define SPI_MISO		PA6
#define SPI_MOSI		PA5
#define SPI_SCLK		PA4

/*
 * Util functions
 */
#define SET_BIT(PORT, BITNUM) ((PORT) |= (1<<(BITNUM)))
#define CLEAR_BIT(PORT, BITNUM) ((PORT) &= ~(1<<(BITNUM)))
#define TOGGLE_BIT(PORT, BITNUM) ((PORT) ^= (1<<(BITNUM)))


#endif /* BOARD_H_ */
