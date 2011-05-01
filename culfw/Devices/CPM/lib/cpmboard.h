#ifndef CPMBOARD_H
#define CPMBOARD_H

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
#define CC1100_INT		INT0
#define CC1100_INTVECT		INT0_vect

#define SPI_PORT		PORTA
#define SPI_DDR			DDRA
#define SPI_MISO		PA6
#define SPI_MOSI		PA5
#define SPI_SCLK		PA4



#endif /* CPMBOARD_H_ */
