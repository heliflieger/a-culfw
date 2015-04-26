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
#include "display.h"


#ifdef HAS_TCM97001
/*
 * Description in header
 */
void analyze_tcm97001(bucket_t *b, uint8_t *datatype, uint8_t *obuf, uint8_t *oby)
{
  if (IS433MHZ && *datatype == 0) {
    if ((b->byteidx != 3 && b->byteidx != 4) || (b->bitidx != 7 && b->bitidx != 3 && b->bitidx != 2) || b->state != STATE_TCM97001) {
		  return;
    }
    
    uint8_t i;
   // uint8_t checksum = 0;
    
    for (i=0;i<b->byteidx;i++) {
      obuf[i]=b->data[i];
    }
    if (7-b->bitidx != 0) {
      obuf[i]=b->data[i];
      i++;
    }
    *oby = i;
    *datatype = TYPE_TCM97001;
  }
}

/*
 * Description in header
 */
void sync_tcm97001(bucket_t *b, pulse_t *hightime, pulse_t *lowtime) 
{
  if (b->sync == 0) {
      b->sync=1;
	    b->zero.hightime = *hightime;
	    b->one.hightime = *hightime;
	    if (*lowtime < 156) { // < 3000=187 156=2500
        //DC('A');
		    b->zero.lowtime = *lowtime*2;
		    b->one.lowtime = *lowtime;
	    } else {
        //DC('B');
		    b->zero.lowtime = *lowtime;
		    b->one.lowtime = *lowtime/2;
	    }
  } 
  
  return;
}

/*
 * Description in header
 */
uint8_t is_tcm97001(bucket_t *b, pulse_t *hightime, pulse_t *lowtime) 
{
  if (IS433MHZ && b->state == STATE_RESET) {
    if ((*hightime < TSCALE(640) && *hightime > TSCALE(410)) &&
				     (*lowtime  < TSCALE(9300) && *lowtime > TSCALE(8600)) ) {
		    OCR1A = 5600; //End of message
			  TIMSK1 = _BV(OCIE1A);
			  b->sync=0;
        
			  b->state = STATE_TCM97001;
			  b->byteidx = 0;
			  b->bitidx  = 7;
			  b->data[0] = 0;  
#ifdef DEBUG_SYNC
        b->syncbit.hightime=*hightime;
        b->syncbit.lowtime=*lowtime;
#endif
		    return 1;
	  }
  }
  return 0;
}

/*
 * Description in header
 */
void addbit_tcm97001(bucket_t *b, pulse_t *hightime, pulse_t *lowtime) 
{
  if (*lowtime < 156) { // < 3000=187 156=2500
    addbit(b,0);
    //b->zero.hightime = makeavg(b->zero.hightime, *hightime);
    //b->zero.lowtime  = makeavg(b->zero.lowtime,  *lowtime);
  } else {
    addbit(b,1);
    //b->one.hightime = makeavg(b->one.hightime, *hightime);
    //b->one.lowtime  = makeavg(b->one.lowtime,  *lowtime);
  }
}

#endif

