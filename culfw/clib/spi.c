#include <avr/io.h>
#include "spi.h"
#include "board.h"

void
spi_init(void)
{
#ifdef PRR0
  PRR0 &= ~_BV(PRSPI);
#endif
  SPI_PORT |= _BV(SPI_SCLK);
  SPI_DDR  |= (_BV(SPI_MOSI) | _BV(SPI_SCLK) | _BV(SPI_SS));
  SPI_DDR  &= ~_BV(SPI_MISO);
  
#ifdef HAS_DOGM
  SPCR = _BV(MSTR) | _BV(SPE) | _BV( SPR0 );
#else
  SPCR  = _BV(MSTR) | _BV(SPE);
  SPSR |= _BV(SPI2X);
#endif

}

uint8_t
spi_send(uint8_t data)
{
  SPDR = data;
  while (!(SPSR & _BV(SPIF)));
  return SPDR;
}
