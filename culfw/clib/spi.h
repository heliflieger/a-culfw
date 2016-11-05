#ifndef _SPI_H
#define _SPI_H

/* prototypes */
void spi_init(void);
uint8_t spi_send(uint8_t data);

#define spi_wait()  while (!(SPSR & _BV(SPIF)))

#endif
