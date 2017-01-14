/* 
 * Copyright 2015 B. Hempel
 *
 * License: 
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

#include "rf_receive_tcm97001.h"

#include <stdbool.h>                    // for false
#include <stdint.h>                     // for uint8_t

#include "fband.h"                      // for IS433MHZ
//#include "display.h"


#ifdef HAS_TCM97001
/*
 * Description in header
 */
void analyze_tcm97001(bucket_t *b, uint8_t *datatype, uint8_t *obuf, uint8_t *oby)
{
  if (IS433MHZ && *datatype == 0) {
     if (b->valCount == 24
        || b->valCount == 28 // Mebus
        || b->valCount == 36
        || b->valCount == 37
        || b->valCount == 42) { // TF Dostmann
      copyData(b->byteidx, b->bitidx, b->data, obuf, oby, false);
      b->state = STATE_TCM97001;
      *datatype = TYPE_TCM97001;
    }
  }
}


#endif

