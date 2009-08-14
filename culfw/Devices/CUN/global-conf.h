//Project specific configurations
#ifndef __GLOBAL_CONF_H__
#define __GLOBAL_CONF_H__

#define ENC28J60CONF_H

//Mac adress definition for enc28j60
#define ETHADDR0		0x00
#define ETHADDR1		0xbd
#define ETHADDR2		0x3b
#define ETHADDR3		0x33
#define ETHADDR4		0x05
#define ETHADDR5		0x71
//Mac adress definition for uip
#define UIP_ETHADDR0    ETHADDR0
#define UIP_ETHADDR1    ETHADDR1
#define UIP_ETHADDR2    ETHADDR2
#define UIP_ETHADDR3    ETHADDR3
#define UIP_ETHADDR4    ETHADDR4
#define UIP_ETHADDR5    ETHADDR5

#define ENC28J60_MAC0 ETHADDR0
#define ENC28J60_MAC1 ETHADDR1
#define ENC28J60_MAC2 ETHADDR2
#define ENC28J60_MAC3 ETHADDR3
#define ENC28J60_MAC4 ETHADDR4
#define ENC28J60_MAC5 ETHADDR5

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
