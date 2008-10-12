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

#ifndef _SPI_H
#define _SPI_H

#include <stdint.h>

#define noinline __attribute__((noinline))

/* prototypes */
void spi_init(void);
void noinline spi_wait_busy(void);
uint8_t noinline spi_send(uint8_t data);

#define SPI_PORT		PORTB
#define SPI_DDR			DDRB
#define SS			PB0
#define MISO			PB3
#define MOSI			PB2
#define SCK			PB1

#define SPI_RES_PORT		PORTE
#define SPI_RES_DDR		DDRE
#define SPI_RES_PIN		PE3

#endif
