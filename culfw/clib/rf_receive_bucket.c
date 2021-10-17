/* 
 * a-culfw
 * Copyright (C) 2015 B. Hempel
 *
 * This program is free software; you can redistribute it and/or modify it under  
 * the terms of the GNU General Public License as published by the Free Software  
 * Foundation; either version 2 of the License, or (at your option) any later  
 * version.
 * 
 * This program is distributed in the hope that it will be useful, but  
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY  
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for  
 * more details.
 * 
 * You should have received a copy of the GNU General Public License along with  
 * this program; if not, write to the  
 * Free Software Foundation, Inc.,  
 * 51 Franklin St, Fifth Floor, Boston, MA 02110, USA
 */

#include "rf_receive_bucket.h"

#include <avr/io.h>                     // for _BV
#include <stdint.h>                     // for uint8_t

packetCheckValues_t packetCheckValues[NUM_SLOWRF];

uint8_t makeavg(uint8_t i, uint8_t j)
{
  return (i+i+i+j)/4;
}

/*
 * Copy the data from bucket to receive buffer
 */
void copyData(uint8_t byteidx, uint8_t bitidx, uint8_t *data, uint8_t *obuf, uint8_t *oby, bool reverseBits) {
      uint8_t i;
      for (i=0;i<byteidx;i++) {
        if (reverseBits) {
          data[i] = ~data[i];
        }
        obuf[i]=data[i];
      }

      if (7-bitidx != 0) {
        if (reverseBits) {
          data[i] = ~data[i] & (0xFF<<(bitidx+1));
        }
        obuf[i]=data[i];
        i++;
      }
      *oby = i;
}


/*
 * Description in header
 */
void addbit(bucket_t *b, uint8_t bit)
{
  if(b->byteidx>=sizeof(b->data)){
    reset_input();
  //  DC('f');
    return;
  }
  if(bit)
    b->data[b->byteidx] |= _BV(b->bitidx);

  if(b->bitidx-- == 0) {           // next byte
    b->bitidx = 7;
    b->data[++b->byteidx] = 0;
  }
  b->valCount = b->valCount + 1;

}

/*
 * Description in header
 */
uint8_t
getbit(input_t *in)
{
  uint8_t bit = (in->data[in->byte] & _BV(in->bit)) ? 1 : 0;
  if(in->bit-- == 0) {
    in->byte++;
    in->bit=7;
  }
  return bit;
}

/*
 * Description in header
 */
uint8_t
getbits(input_t* in, uint8_t nbits, uint8_t msb)
{
  uint8_t ret = 0, i;
  for (i = 0; i < nbits; i++) {
    if (getbit(in) )
      ret = ret | _BV( msb ? nbits-i-1 : i );
  }
  return ret;
}
