//Project specific configurations
#ifndef __GLOBAL_CONF_H__
#define __GLOBAL_CONF_H__

// Default MAC is initialized to 0
#define ENC28J60_MAC0 0
#define ENC28J60_MAC1 0
#define ENC28J60_MAC2 0
#define ENC28J60_MAC3 0
#define ENC28J60_MAC4 0
#define ENC28J60_MAC5 0


#define ENC28J60CONF_H

// ENC28J60 SPI port
#define ENC28J60_SPI_PORT		PORTB
#define ENC28J60_SPI_DDR		DDRB
#define ENC28J60_SPI_SCK		PB1
#define ENC28J60_SPI_MOSI		PB2
#define ENC28J60_SPI_MISO		PB3
#define ENC28J60_SPI_SS			PB0
// ENC28J60 control port
#define ENC28J60_CONTROL_PORT	        PORTD
#define ENC28J60_CONTROL_DDR	        DDRD
#define ENC28J60_CONTROL_CS		PD7

//Include uip.h gives all the uip configurations in uip-conf.h
#include "uip.h"

#endif /*__GLOBAL_CONF_H__*/
