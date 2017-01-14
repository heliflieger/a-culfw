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

#include "rf_receive_it.h"

#include <stdbool.h>                    // for false, true
#include <stdint.h>                     // for uint8_t

#include "fband.h"                      // for IS433MHZ


#ifdef HAS_IT


/*
 * Description in header
 */
void analyze_intertechno(bucket_t *b, uint8_t *datatype, uint8_t *obuf, uint8_t *oby)
{
  if (IS433MHZ && *datatype == 0 && b->state == STATE_SYNC_PACKAGE) {
    if ((b->valCount == 24 // IT V1
      || b->valCount == 28 // HE800
      || b->valCount == 57 // HE_EU
      || b->valCount == 65 // IT V3
      || b->valCount == 73) // IT V3 Dim
      && b->one.lowtime < TSCALE(2800) && b->two.lowtime < TSCALE(2000)) {
      // IT V1 (48), V3 (128)
      if (b->valCount == 24) {
        copyData(b->byteidx, b->bitidx, b->data, obuf, oby, true);
        // inverse the bits in bucket
        b->state = STATE_IT;
#ifdef HAS_HOMEEASY
      } else if (b->valCount == 28 || b->valCount == 57) {
        // HE 800
        // inverse the bits in bucket
        copyData(b->byteidx, b->bitidx, b->data, obuf, oby, false);
        b->state = STATE_HE;
#endif
      } else if (b->valCount == 65 || b->valCount == 73) {
        // IT V3
        if (b->zero.lowtime > TSCALE(1500)) {
          // has startbit
          // remove the startbit from bucket.
          // inverse the bits in bucket
          uint8_t i;
          for (i=0;i<b->byteidx;i++)  {
            if (i+1==b->byteidx) {
              obuf[i]= b->data[i]<<1;
              if (b->bitidx != 7) {
                obuf[i+1] = b->data[i+1];
                obuf[i] = obuf[i] + ((b->data[i+1]>>7) & 1);
              }
            } else {    
              obuf[i]= ((b->data[i])<<1) + (((b->data[i+1])>>7) & 1);
            }
          }
          *oby = i;
          b->state = STATE_ITV3;
        }
        
      } 
        *datatype = TYPE_IT;
      return;
    }
  }

}

#endif

