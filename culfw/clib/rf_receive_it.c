/* 
 * Copyright by B.Hempel
 * License: GPL v2
 */

#include "rf_receive_it.h"
#include "display.h"

#ifdef HAS_IT
/*
 * Description in header
 */
void analyze_intertechno(bucket_t *b, uint8_t *datatype, uint8_t *obuf, uint8_t *oby)
{
  if (IS433MHZ && *datatype == 0) {
    if ((b->state != STATE_IT || b->byteidx != 3 || b->bitidx != 7) 
        && (b->state != STATE_ITV3 || (b->byteidx != 8 && b->byteidx != 9) || b->bitidx != 7)) {
        return;
    }

    uint8_t i;
    for (i=0;i<b->byteidx;i++)
      obuf[i]=b->data[i];

    *oby = i;
    *datatype = TYPE_IT;
  }
}


/*
 * Description in header
 */
uint8_t sync_intertechno(bucket_t *b, pulse_t *hightime, pulse_t *lowtime) 
{
   if(IS433MHZ && (b->state == STATE_IT || b->state == STATE_ITV3)) {
    if (*lowtime > TSCALE(3000)) {
        b->sync = 0;
        return 1;
    }
    if (b->sync == 0) {
      if (*lowtime > TSCALE(2400)) { 
        // this sould be the start bit for IT V3
        b->state = STATE_ITV3;
        TCNT1 = 0;                          // restart timer
        return 1;
      } else if (b->state == STATE_ITV3) {
        b->sync=1;
        if (*lowtime-1 > *hightime) {
          b->zero.hightime = *hightime; 
		      b->zero.lowtime = *lowtime;
        } else {
          b->zero.hightime = *hightime; 
		      b->zero.lowtime = *hightime*5;
        }
        b->one.hightime = *hightime;
	      b->one.lowtime = *hightime;
      } else {
        b->sync=1;
        if (*hightime*2>*lowtime) {
          // No IT, because times to near
          b->state = STATE_RESET;
          return 1;
        }
        b->zero.hightime = *hightime; 
        b->zero.lowtime = *lowtime+1;
        b->one.hightime = *lowtime+1;
        b->one.lowtime = *hightime;
      }
    }
  }
  return 0;
}

/*
 * Description in header
 */
uint8_t is_intertechno(bucket_t *b, pulse_t *hightime, pulse_t *lowtime) 
{
  if(IS433MHZ && (*hightime < TSCALE(600) && *hightime > TSCALE(140)) &&
     (*lowtime < TSCALE(12000) && *lowtime > TSCALE(6000)) ) {
    OCR1A = 3100; // end of message
    TIMSK1 = _BV(OCIE1A);
    b->sync=0;
    b->state = STATE_IT;
    b->byteidx = 0;
    b->bitidx  = 7;
    b->data[0] = 0;   
#ifdef DEBUG_SYNC
    b->syncbit.hightime=*hightime;
    b->syncbit.lowtime=*lowtime;
#endif
    return 1;
	}
  return 0;
}

/*
 * Description in header
 */
void addbit_intertechno_v3(bucket_t *b, pulse_t *hightime, pulse_t *lowtime) 
{
  if (IS433MHZ) {
    if(*lowtime > TDIFF + *hightime) {
      addbit(b, 1);
    } else {
      addbit(b, 0);
    }
  }
}

#endif

