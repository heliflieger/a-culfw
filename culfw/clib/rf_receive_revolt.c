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

#include "rf_receive_revolt.h"

#include <avr/io.h>                     // for _BV
#include <stdint.h>                     // for uint8_t

#include "fband.h"                      // for IS433MHZ
#ifdef USE_HAL
#include "hal.h"
#endif
#include "rf_mode.h"

#ifdef HAS_REVOLT
/*
 * Description in header
 */
void analyze_revolt(bucket_t *b, uint8_t *datatype, uint8_t *obuf, uint8_t *oby)
{
  if (IS433MHZ && *datatype == 0) {
    
    if (b->byteidx != 12 || b->state != STATE_REVOLT || b->bitidx != 0)
      return;

    uint8_t sum=0;
    uint8_t i;
    for (i=0;i<11;i++) {
      sum+=b->data[i];
      obuf[i]=b->data[i];
    }

    *oby = i;
 
    if (sum!=b->data[11])
        return; // failed
    *datatype = TYPE_REVOLT;
  }
}

/*
 * Description in header
 */
bool is_revolt(bucket_t *b, pulse_t *hightime, pulse_t *lowtime) 
{
  if (IS433MHZ && (*hightime > TSCALE(8900)) && (*hightime < TSCALE(12000)) &&
      (*lowtime  > TSCALE(150))   && (*lowtime  < TSCALE(540))) {
    // Revolt
    b->zero.hightime = 9; // = 144
    b->zero.lowtime = 14; // = 224
    b->one.hightime = 16; // = 256
    b->one.lowtime = 14;  // = 224
    b->sync=1;
    b->state = STATE_REVOLT;
    b->byteidx = 0;
    b->bitidx  = 7;
    b->data[0] = 0;
    #ifdef SAM7
    HAL_timer_set_reload_register(CC_INSTANCE,SILENCE/8*3);
    #elif defined STM32
    HAL_timer_set_reload_register(CC_INSTANCE,SILENCE);
    #else
        OCR1A = SILENCE;
    #endif
    #ifdef USE_HAL
        hal_enable_CC_timer_int(CC_INSTANCE,TRUE);
    #else
        TIMSK1 = _BV(OCIE1A);
    #endif
    return true;
  }
  return false;
}

/*
 * Description in header
 */
void addbit_revolt(bucket_t *b, pulse_t *hightime, pulse_t *lowtime) 
{
  if (IS433MHZ) {
    if ((*hightime < 11)) { // 176
        addbit(b,0);
        //b->zero.hightime = makeavg(b->zero.hightime, *hightime);
        //b->zero.lowtime  = makeavg(b->zero.lowtime,  *lowtime);
      } else {
        addbit(b,1);
        //b->one.hightime = makeavg(b->one.hightime, *hightime);
        //b->one.lowtime  = makeavg(b->one.lowtime,  *lowtime);
      }
  }
}

#endif


