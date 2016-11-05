#include <avr/io.h>
#include "spi.h"
#include "board.h"

__attribute__((weak)) void spi_init(void)
{
#ifdef SAM7
  AT91C_BASE_PMC->PMC_PCER = (1 << AT91C_ID_SPI0);

  AT91C_BASE_PIOA->PIO_PPUER =  SPI_MISO | SPI_MOSI | SPI_SCLK;
  AT91C_BASE_PIOA->PIO_ASR =  SPI_MISO | SPI_MOSI | SPI_SCLK;
  AT91C_BASE_PIOA->PIO_PDR =  SPI_MISO | SPI_MOSI | SPI_SCLK;


  AT91C_BASE_SPI0->SPI_CR = AT91C_SPI_SWRST;
  AT91C_BASE_SPI0->SPI_CR = AT91C_SPI_SWRST;

  AT91C_BASE_SPI0->SPI_CR = AT91C_SPI_SPIEN;
  AT91C_BASE_SPI0->SPI_MR= AT91C_SPI_MSTR| AT91C_SPI_MODFDIS;
  AT91C_BASE_SPI0->SPI_CSR[0] = AT91C_SPI_NCPHA | (48<<8);
#elif defined STM32

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

__attribute__((weak)) uint8_t spi_send(uint8_t data)
{
#ifdef SAM7
  // Send data
  while ((AT91C_BASE_SPI0->SPI_SR & AT91C_SPI_TXEMPTY) == 0);
  AT91C_BASE_SPI0->SPI_TDR = data;
  while ((AT91C_BASE_SPI0->SPI_SR & AT91C_SPI_RDRF) == 0);
  return AT91C_BASE_SPI0->SPI_RDR & 0xFF;
#elif defined STM32
  return 0;
#else
  SPDR = data;
  while (!(SPSR & _BV(SPIF)));
  return SPDR;
#endif
}
