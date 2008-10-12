/* vim:fdm=marker ts=4 et ai
 * {{{
 *
 * (c) by Alexander Neumann <alexander@bumpern.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * For more information on the GPL, please go to:
 * http://www.gnu.org/copyleft/gpl.html
 }}} */

#include <avr/io.h>
#include "spi.h"

void spi_init(void)
/* {{{ */ {

     SPI_RES_DDR  |= _BV(SPI_RES_PIN);   // RESET
     SPI_RES_PORT &= ~_BV(SPI_RES_PIN);  // reset all devices
     
     // SCK auf Hi setzen
     SPI_PORT |= 1<<SCK;
     // MOSI, SCK, SS als Output
     SPI_DDR  |= 1<<MOSI | 1<<SCK | 1<<SS; // mosi, sck, ss output
     // MISO als Input
     SPI_DDR  &= ~( 1<<MISO ); // miso input
     
     // Master mode
     SPCR = 1<<MSTR | 1<<SPE;

//     SPCR |= _BV(SPR0);

     SPSR |= _BV(SPI2X);
     
     // un-Reset all devices
     SPI_RES_PORT |= _BV(SPI_RES_PIN);
} /* }}} */

void noinline spi_wait_busy(void)
/* {{{ */ {

    while (!(SPSR & _BV(SPIF)));

} /* }}} */

uint8_t noinline spi_send(uint8_t data)
/* {{{ */ {

    SPDR = data;
    spi_wait_busy();

    return SPDR;

} /* }}} */
