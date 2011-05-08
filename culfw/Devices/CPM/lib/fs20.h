/* vim:fdm=marker ts=4 et ai
 * {{{
 *
 *          fs20 sender implementation
 *
 * (c) by Alexander Neumann <alexander@bumpern.de>
 * (c) amendements by Maz Rashid
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


#ifndef _FS20_H
#define _FS20_H

/*
 * for future use, currenty not used as we are at the moment only interested in FS20-Telegrams

typedef enum {
	FS20,
	FHT,
	EM,
	KS300,
	HMS
} telegramtype;

typedef struct {
	telegramtype type;
	void* data;
} telegram_t;
*/
typedef struct
{
  uint8_t type;			// 'F': FS20, 'T': FHT
  uint16_t housecode;
  uint8_t button;
  uint8_t cmd;
  uint8_t bytes;
  uint8_t data[6];
} fstelegram_t;


/* public prototypes */
void fs20_init (void);
void fs20_rxon (void);
void fs20_idle (void);
void fs20_resetbuffer (void);
void fs20_send (uint8_t[], uint8_t len);

uint8_t fs20_readFS20Telegram (fstelegram_t * t);
uint8_t fs20_busy(void);
  
#endif
