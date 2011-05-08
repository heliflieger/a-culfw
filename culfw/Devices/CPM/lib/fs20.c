/* vim:fdm=marker ts=4 et ai
 * {{{
 *
 *          fs20 sender implementation
 *
 * (c) by Alexander Neumann <alexander@bumpern.de>
 * amendments by Maz Rashid (maz at amazers dot net) and Boris Neubert (omega at online dot de)
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
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#include <avr/pgmspace.h>
#include <util/parity.h>
#include "board.h"
#include "util.h"
#include "fs20.h"
#include "cc1100.h"
#include <string.h>

#define EDGETIMEOUT 2500
#define AVG_SIGNAL 1

volatile uint16_t bl, bh;

typedef struct
{
  uint16_t high, low;
} bit_t;

typedef struct
{
  uint8_t dec[10];
  uint8_t bytes, type;
} airmsg_t;

typedef struct
{
  uint8_t bits, sync, state;
  uint8_t data[20];
  bit_t bit[2];
#ifdef AVG_SIGNAL
  bit_t sum[2];
#endif
} airdata_t;

#define AD_Q_MX 3

volatile airdata_t ad;
airdata_t adq[AD_Q_MX];
uint8_t adc = 0;

/*

AIRDATA functions

*/

void
AD_SET_BIT (airdata_t * d, uint8_t b)
{
  d->data[b >> 3] |= _BV (b & 7);
}

void
AD_PUSH (airdata_t * d, uint8_t b)
{

  if (b)
    AD_SET_BIT (d, d->bits);
  d->bits++;
}

void
AD_PUSH_SYNC (airdata_t * d, uint8_t b)
{

  for (uint8_t i = 0; i < b; i++)
    AD_PUSH (d, 0);

  AD_PUSH (d, 1);
}

void
AD_PUSH_BYTE (airdata_t * d, uint8_t b)
{

  for (uint8_t i = 0; i < 8; i++)
    AD_PUSH (d, bit_is_set (b, 7 - i));

}

uint8_t
AD_BIT_IS_SET (airdata_t * d, uint8_t b)
{
  return bit_is_set (d->data[b >> 3], b & 7);
}

uint8_t
AD_SHL (airdata_t * d)
{
  uint8_t val = 0;

  if (d->bits)
    {
      val = d->data[0] & 1;

      d->bits--;
      d->data[0] = d->data[0] >> 1;

      // move the bits
      for (uint8_t b = 0; b < (d->bits >> 3); b++)
	{
	  if (bit_is_set (d->data[b + 1], 0))
	    d->data[b] = d->data[b] | 0x80;
	  d->data[b + 1] = d->data[b + 1] >> 1;
	}

    }

  return val;
}

uint16_t
AD_POP (airdata_t * d, uint8_t b, uint8_t msb)
{
  uint16_t val = 0;

  for (uint8_t i = 0; i < b; i++)
    {

      if (AD_SHL (d))
	val = val | _BV (msb ? b - i - 1 : i);

    }


  return val;
}

void
AD_DUMP (airdata_t * d, uint8_t b)
{
  for (uint8_t i = 0; i < b; i++)
    {
      if (AD_BIT_IS_SET (d, i))
	{
	  printf_P (PSTR ("1"));
	}
      else
	{
	  printf_P (PSTR ("0"));
	}
    }

  printf_P (PSTR ("\r\n"));
}

void
fs20_init (void)
{

  // INT0 - Incoming signal
  CLEAR_BIT (DDRB, PB2);
  MCUCR |= _BV (ISC00);		// any egde

  GIFR |= _BV (INTF0);		// clear  INT
  GIMSK |= _BV (INT0);		// enable INT

  // Timer1 @ 1MHz == 1us
  TCNT1 = 0;
  TCCR1A = 0;
  TCCR1B = _BV (CS11);		// prescale 8 - 1usec
//     OCR1A  = BITTEST;     // BitTest time
  OCR1B = EDGETIMEOUT;		// Timeout
  TIMSK1 = 0;			// _BV( OCIE1A ) | _BV( OCIE1B ); // start disabled!

  bl = 0;
  bh = 0;

  memset ((airdata_t *) (&ad), 0, sizeof (ad));

}				/* }}} */

void
fs20_rxon (void)
{
  ccRX ();
}

void
fs20_idle (void)
{
  ccIdle ();
}


/*
   RECEIVER
*/


ISR (TIM1_COMPA_vect)
{
  CLEAR_BIT (CC1100_OUT_PORT, CC1100_OUT_PIN);
  TIFR1 |= _BV (OCF1A);
}

ISR (TIM1_COMPB_vect)
{
//   If we end up here we run into a timeout, no egdes anymore ...

  if (ad.state < 100)
    {
      // RX
/*
	  if ((cc1100_readReg( CC1100_MARCSTATE ) & 0x1f) != 13) {
	       ccStrobe( CC1100_SRX );
	       TCNT1 = 0;
	       OCR1B = 1000; // 1ms
	       TIFR1 |= _BV( OCF1B );
//	       printf_P( PSTR("RX ") );
	       return;

	  }
*/

      TIMSK1 = 0;		// stop any timer interrupts
      OCR1B = EDGETIMEOUT;
      SET_BIT (GIFR, INTF0);	// clear  INT
      SET_BIT (GIMSK, INT0);	// enable INT

      if (ad.bits > 20 && adc < AD_Q_MX)
	{
	  memcpy (&adq[adc], (airdata_t *) & ad, sizeof (ad));
	  adc++;
	}

      bl = 0;
      bh = 0;
      memset ((airdata_t *) (&ad), 0, sizeof (ad));

    }
  else
    {
      // TX
      // process next bit:

//        printf_P( PSTR("%d "), TCNT1 );

      TCNT1 = 0;

      CLEAR_BIT (CC1100_OUT_PORT, CC1100_OUT_PIN);

      // check if CC is in TX mood
      if ((cc1100_readReg (CC1100_MARCSTATE) & 0x1f) != 19)
	{
	  ccStrobe (CC1100_STX);
	  OCR1B = 1000;		// 1ms - come again later
	  TIFR1 |= _BV (OCF1B);
//             printf_P( PSTR("TX ") );
	  return;
	}

      // still data to be sent?
      if (ad.sync < ad.bits)
	{

	  SET_BIT (CC1100_OUT_PORT, CC1100_OUT_PIN);

	  if (AD_BIT_IS_SET ((airdata_t *) & ad, ad.sync))
	    {
	      // "1"
	      OCR1A = ad.bit[1].high;
	      OCR1B = ad.bit[1].low;
//                  printf_P( PSTR("|") );

	    }
	  else
	    {
	      // "0"
	      OCR1A = ad.bit[0].high;
	      OCR1B = ad.bit[0].low;
//                  printf_P( PSTR("o") );

	    }

	  SET_BIT (TIMSK1, OCIE1A);
	  SET_BIT (TIFR1, OCF1A);

	  ad.sync++;

	}
      else
	{
	  // message complete

	  CLEAR_BIT (TIMSK1, OCIE1A);

	  if (--ad.state < 100)
	    {
	      // finish!
	      ccStrobe (CC1100_SIDLE);
	      memset ((airdata_t *) & ad, 0, sizeof (ad));
	      OCR1B = 50000;	// give time to relax ...

	    }
	  else
	    {
	      // wait and repeat:
	      ad.sync = 0;
	      OCR1B = 10000;	// 10ms break
//                  printf_P( PSTR("\r\n") );

	    }

	}

    }

  TIFR1 |= _BV (OCF1B);
}

#define BITWIN 200

uint8_t
looks_like (bit_t a, bit_t b)
{
  int16_t dbl = a.low - b.low;
  int16_t dbh = a.high - b.high;

  int16_t dp = (a.low + a.high) - (b.low + b.high);

  if (dp < BITWIN && dp > -BITWIN &&
      dbl < BITWIN && dbl > -BITWIN && dbh < BITWIN && dbh > -BITWIN)

    return 1;

  return 0;
}

uint8_t
looks_like_s (bit_t a, uint16_t h, uint16_t l)
{
  bit_t b;

  b.low = l;
  b.high = h;

  return looks_like (a, b);
}



ISR (INT0_vect)
{

  //led_blink(1);
  if (ad.state == 99)
    {				// HMS processing

      if (TCNT1 < 750)
	return;

      if (TCNT1 > 1250)
	{
	  ad.state = 0;
	}

    }

  uint16_t c = TCNT1;		// save the timer

  if (bit_is_set (PINB, PB2))
    {
      // rising

      TCNT1 = 0;

      if (ad.state == 99)
	{
	  AD_PUSH ((airdata_t *) & ad, 0);
//             printf_P( PSTR("o"));

	}
      else if (bh)
	{			// have full period covered?
	  bl = c - bh;

	  switch (ad.state)
	    {

	    case 1:		// collect SYNC bits of same kind

	      // check if this bit is within bit window
	      if (looks_like_s (ad.bit[0], bh, bl))
		{

		  // looks like the same

		  ad.sync++;

#ifdef AVG_SIGNAL
		  ad.sum[0].high += bh;
		  ad.sum[0].low += bl;

		  ad.bit[0].high = ad.sum[0].high / ad.sync;
		  ad.bit[0].low = ad.sum[0].low / ad.sync;
#endif

		  break;


		}
	      else if (ad.sync >= 16
		       && (ad.bit[0].high + ad.bit[0].low) > 1600)
		{
		  ad.state = 99;
//                       printf_P( PSTR("HMS ") );

		  break;

		}
	      else if (ad.sync >= 6)
		{


		  // store signal for "1"
		  ad.bit[1].high = bh;
		  ad.bit[1].low = bl;

/*
			 printf_P( PSTR("S 0=%d/%d-%d 1=%d/%d-%d "),
				   ad.bit[0].high, ad.bit[0].low, ad.bit[0].high+ad.bit[0].low,
				   ad.bit[1].high, ad.bit[1].low, ad.bit[1].high+ad.bit[1].low);
*/

#ifdef AVG_SIGNAL
		  ad.sum[1].high = bh;
		  ad.sum[1].low = bl;
#endif
		  ad.sync = 1;

		  ad.state = 2;

		  break;

		}
	      else
		{

/*
			 // do "state 0" stuff
			 memset( &ad, 0, sizeof( ad ));
			 ad.bit[0].high = bh;
			 ad.bit[0].low  = bl;
#ifdef AVG_SIGNAL
			 ad.sum[0].high = bh;
			 ad.sum[0].low  = bl;
#endif
			 ad.sync   = 1;
			 ad.state  = 1;

*/
		  ad.state = 0;

		}

	    case 2:		// collect SYNC bits of same kind

	      // check if this bit is within bit window
	      if (looks_like_s (ad.bit[0], bh, bl))
		{

		  AD_PUSH ((airdata_t *) & ad, 0);
//                       printf_P( PSTR("0"));
		  break;

		}
	      else if (looks_like_s (ad.bit[1], bh, bl))
		{

		  AD_PUSH ((airdata_t *) & ad, 1);
//                       printf_P( PSTR("1"));

#ifdef AVG_SIGNAL
		  if (ad.sync++ <= 6)
		    {

		      ad.sum[1].high += bh;
		      ad.sum[1].low += bl;

		      ad.bit[1].high = ad.sum[1].high / ad.sync;
		      ad.bit[1].low = ad.sum[1].low / ad.sync;

		    }
#endif
		  break;

		}
	      else
		{

//                       printf_P( PSTR("?\r\n"));

/*
			 // do "state 0" stuff
			 memset( &ad, 0, sizeof( ad ));
			 ad.bit[0].high = bh;
			 ad.bit[0].low  = bl;
#ifdef AVG_SIGNAL
			 ad.sum[0].high = bh;
			 ad.sum[0].low  = bl;
#endif
			 ad.sync   = 1;
			 ad.state  = 1;
*/
		  ad.state = 0;

		}

	    case 0:		// INIT
	      memset ((airdata_t *) & ad, 0, sizeof (ad));

	      ad.bit[0].high = bh;
	      ad.bit[0].low = bl;
#ifdef AVG_SIGNAL
	      ad.sum[0].high = bh;
	      ad.sum[0].low = bl;
#endif

	      ad.sync = 1;
	      ad.state = 1;
	      break;

	    }

	  bh = 0;
	}


    }
  else
    {
      // falling

      if (ad.state == 99)
	{
	  TCNT1 = 0;
	  AD_PUSH ((airdata_t *) & ad, 1);

//             printf_P( PSTR("|"));

	}
      else
	{
	  bh = c;
	  bl = 0;

	}

    }

  TIFR1 |= _BV (OCF1A) | _BV (OCF1B);	// clear Bittest and Timeout
  SET_BIT (TIMSK1, OCIE1B);

//     LED_TOGGLE();

  GIFR |= _BV (INTF0);
}


/*
   ANALYSER
*/

/*
  FS format checker ...
*/
uint8_t
check_fs (airdata_t * d, airmsg_t * am)
{

  am->bytes = 0;
  am->type = 0;

  if (d->bits < 45)
    return 0;

  uint8_t crc = 0;

  // read 5 bytes with parity ...
  for (uint8_t i = 0; i < 4; i++)
    {

      am->dec[i] = AD_POP (d, 8, 1);
      if (parity_even_bit (am->dec[i]) != AD_SHL (d))
	return 0;

      crc += am->dec[i];
      am->bytes++;

    }

  // Extentions bit set?
  if (bit_is_set (am->dec[3], 5))
    {
      am->dec[4] = AD_POP (d, 8, 1);
      if (parity_even_bit (am->dec[4]) != AD_SHL (d))
	{
//             printf_P( PSTR("xPE"));
	  return 0;
	}

      crc += am->dec[4];
      am->bytes++;
    }

  // Read crc
  uint8_t CRC = AD_POP (d, 8, 1);
  if (parity_even_bit (CRC) != AD_SHL (d))
    {
//        printf_P( PSTR("cPE"));
      return 0;
    }

  if (CRC - crc == 6)
    {
      am->type = 'F';
      //printf_P( PSTR("F") );
    }
  else if (CRC - crc == 12)
    {
      am->type = 'T';
      //printf_P( PSTR("T") );
    }
  else
    {
//        printf_P( PSTR("crc %d\r\n"), CRC-crc );
      return 0;
    }

  return 1;
}

/*
  EM format checker ...
*/
uint8_t
check_em (airdata_t * d, airmsg_t * am)
{

//     printf_P( PSTR("e") );

  am->bytes = 0;

  if (d->bits < 89)
    {
//        printf_P( PSTR("eless %d\r\n"), d->bits);
      return 0;
    }

  uint8_t crc = 0;

  // read 9 bytes with stopbit ...
  for (uint8_t i = 0; i < 9; i++)
    {

      am->dec[i] = AD_POP (d, 8, 0);
      if (!AD_SHL (d))
	{
	  printf_P (PSTR ("eSB"));
	  return 0;
	}


      crc = crc ^ am->dec[i];
      am->bytes++;

    }

  // Read crc
  uint8_t CRC = AD_POP (d, 8, 0);

  if (CRC != crc)
    {
      printf_P (PSTR ("ecrc %d %d\r\n"), CRC, crc);
      return 0;
    }

  printf_P (PSTR ("E"));
  for (uint8_t i = 0; i < am->bytes; i++)
    {
      printf_P (PSTR ("%02X"), am->dec[i]);
    }

  printf_P (PSTR ("\r\n"));

  return 1;
}



/*
  HMS format checker ...
*/
uint8_t
check_hms (airdata_t * d, airmsg_t * am)
{

//     printf_P( PSTR("m"));

  am->bytes = 0;

  if (d->bits < 69)
    return 0;

  uint8_t crc = 0;

  // read 6 bytes with parity and stopbit...
  for (uint8_t i = 0; i < 6; i++)
    {

      am->dec[i] = AD_POP (d, 8, 0);
      if (parity_even_bit (am->dec[i]) != AD_SHL (d))
	{
//             printf_P( PSTR("PE\r\n"));
	  return 0;
	}

      if (AD_SHL (d))
	{
//             printf_P( PSTR("SB\r\n"));
	  return 0;
	}

      crc = crc ^ am->dec[i];
      am->bytes++;

    }

  // Read crc
  uint8_t CRC = AD_POP (d, 8, 0);
  if (parity_even_bit (CRC) != AD_SHL (d))
    {
//        printf_P( PSTR("cPE\r\n"));
      return 0;
    }

  if (crc != CRC)
    {
//        printf_P( PSTR("CRC\r\n"));
      return 0;
    }

  printf_P (PSTR ("M"));
  for (uint8_t i = 0; i < am->bytes; i++)
    {
      printf_P (PSTR ("%02X"), am->dec[i]);
    }

  printf_P (PSTR ("\r\n"));

  return 1;
}

/*
  KS format checker ...
*/
uint8_t
check_ks (airdata_t * d, airmsg_t * am)
{

//     printf_P( PSTR("k"));

  am->bytes = 0;

  if (d->bits < 35)
    return 0;

  uint8_t crc = 0;
  uint8_t sum = 0;
  uint8_t i = 0;
  uint8_t b = 0;
  uint8_t B = 0;

//     AD_DUMP( d, 20 );

  while (d->bits > 4)
    {

      B = AD_POP (d, 4, 0);

//        if (d->bits>5) {
      crc = crc ^ B;
      sum = sum + B;
//        }

      if (i++)
	{
	  am->dec[am->bytes++] = b | B;
	  i = 0;
	}
      else
	{
	  b = B << 4;
	}

      if (!AD_SHL (d))
	{
//             printf_P( PSTR("SB\r\n"));
	  return 0;
	}

    }

  if (i)
    am->dec[am->bytes++] = b;

  if (crc)
    {
      printf_P (PSTR ("kcrc %d sum %d\r\n"), crc, sum & 0x0f);
      return 0;
    }


  printf_P (PSTR ("K"));
  for (uint8_t i = 0; i < am->bytes; i++)
    {
      printf_P (PSTR ("%02X"), am->dec[i]);
    }

  printf_P (PSTR ("\r\n"));

  return 1;
}


uint8_t
analyse (airdata_t * d)
{
  airdata_t D;
  airmsg_t am;

  memcpy (&D, d, sizeof (D));
  if (check_fs (&D, &am))
    return 1;

  memcpy (&D, d, sizeof (D));
  if (check_hms (&D, &am))
    return 2;

  memcpy (&D, d, sizeof (D));
  if (check_ks (&D, &am))
    return 3;

  memcpy (&D, d, sizeof (D));
  if (check_em (&D, &am))
    return 4;

  return 0;
}

/**
 * reset buffer will set the buffer counter for the received messages to 0 to get rid of old queued messages.
 */
void
fs20_resetbuffer (void)
{
  cli ();
  adc = 0;
  sei ();
}

airdata_t
readQueue (void)
{
  airdata_t d;
  if (adc > 0)			// should be always the case
    {
      cli ();

      memcpy (&d, &adq[0], sizeof (ad));
      adc--;
      memcpy (&adq[0], &adq[1], sizeof (ad) * adc);

      sei ();
    }

  return d;
}

/**
 * reads and converts the next fs20 airdata in the message queue and puts it into the provided variable t.
 * Any other messages on the queue (even fht) will be discarded.
 */
uint8_t
fs20_readFS20Telegram (fstelegram_t * t)
{
  // check prerequisits
  if (adc == 0)
    return 0;

  // take next airdata from the queue
  airdata_t d = readQueue ();
  airmsg_t am;
  if (check_fs (&d, &am) && (am.type == 'F') && (am.bytes > 3))
    {
      t->type = am.type;
      t->housecode = (am.dec[0] << 8) | am.dec[1];
      t->button = am.dec[2];
      t->cmd = am.dec[3];
      if (am.bytes > 4)
	{
	  t->bytes = am.bytes - 4;
	  for (uint8_t i = 0; i < t->bytes; i++)
	    t->data[i] = am.dec[i + 4];
	}

      //led_blink (2);
      return 1;
    }

  return 0;
}

void
Analyze_Task (void)
{


  airdata_t d;

  if (adc == 0)
    return;

  cli ();

  memcpy (&d, &adq[0], sizeof (ad));
  adc--;
  memcpy (&adq[0], &adq[1], sizeof (ad) * adc);

  sei ();

  analyse(&d);
  /*
  if (analyse (&d))
    LED_ON ();*/

}

/*
  SENDING
*/


void
fs20_send (uint8_t data[], uint8_t len)
{

  // init time 1us
  // switch pegel 'H'
  // OCR1A will force pegel Low
  // OCR1B will process next bit ...

  CLEAR_BIT (GIMSK, INT0);	// disable RX-INT
  TIMSK1 = 0;			// disable Timer INT

  ccStrobe (CC1100_SIDLE);

  memset ((airdata_t *) & ad, 0, sizeof (ad));

  // a simple FS20 message:

  // define high, lows for bits
  ad.bit[0].high = 400;
  ad.bit[0].low = 800;
  ad.bit[1].high = 600;
  ad.bit[1].low = 1200;

  ad.state = 102;		// ( repeat 3 times )

  AD_PUSH_SYNC ((airdata_t *) & ad, 12);

  uint8_t crc = 6;
  for (uint8_t i = 0; i < len; i++)
    {

      AD_PUSH_BYTE ((airdata_t *) & ad, data[i]);
      crc += data[i];
      AD_PUSH ((airdata_t *) & ad, parity_even_bit (data[i]));

    }

  AD_PUSH_BYTE ((airdata_t *) & ad, crc);
  AD_PUSH ((airdata_t *) & ad, parity_even_bit (crc));
  AD_PUSH ((airdata_t *) & ad, 1);	// EOT

//     AD_DUMP( &ad, 24 );

  TCNT1 = 0;
  OCR1B = 1000;			// start after a 1ms
  SET_BIT (TIFR1, OCF1B);
  SET_BIT (TIMSK1, OCIE1B);
}

uint8_t fs20_busy(void) {
  return (ad.state>=100);
}

/*

0000000000001
1001 1100 0
1101 0010 0
0000 0000 0
0100 1000 0
0011 1001 0

*/
