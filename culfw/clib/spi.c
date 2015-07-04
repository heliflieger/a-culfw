#include <avr/io.h>
#include "spi.h"
#include "board.h"

void
spi_init(void)
{
#ifdef ARM
  AT91C_BASE_PMC->PMC_PCER = (1 << AT91C_ID_SPI0);

  AT91C_BASE_PIOA->PIO_PPUER =  SPI_MISO | SPI_MOSI | SPI_SCLK;
  AT91C_BASE_PIOA->PIO_ASR =  SPI_MISO | SPI_MOSI | SPI_SCLK;
  AT91C_BASE_PIOA->PIO_PDR =  SPI_MISO | SPI_MOSI | SPI_SCLK;


  AT91C_BASE_SPI0->SPI_CR = AT91C_SPI_SWRST;
  AT91C_BASE_SPI0->SPI_CR = AT91C_SPI_SWRST;

  AT91C_BASE_SPI0->SPI_CR = AT91C_SPI_SPIEN;
  AT91C_BASE_SPI0->SPI_MR= AT91C_SPI_MSTR| AT91C_SPI_MODFDIS;
  AT91C_BASE_SPI0->SPI_CSR[0] = AT91C_SPI_NCPHA | (48<<8);
#else
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
#endif

}

uint8_t
spi_send(uint8_t data)
{
#ifdef ARM
  // Send data
  while ((AT91C_BASE_SPI0->SPI_SR & AT91C_SPI_TXEMPTY) == 0);
  AT91C_BASE_SPI0->SPI_TDR = data;
  while ((AT91C_BASE_SPI0->SPI_SR & AT91C_SPI_RDRF) == 0);
  return AT91C_BASE_SPI0->SPI_RDR & 0xFF;
#else
  SPDR = data;
  while (!(SPSR & _BV(SPIF)));
  return SPDR;
#endif
}
