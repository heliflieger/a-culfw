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
        && (b->state != STATE_ITV3 || (b->byteidx != 8 && b->byteidx != 9) || b->bitidx != 7)
#ifdef HAS_HOMEEASY
        && (b->state != STATE_ITV3 || b->byteidx != 7 ||  b->bitidx != 6) // unknown protocol (mybe HE EU)
        && (b->state != STATE_ITV3 || b->byteidx != 3 ||  b->bitidx != 3) // HE800 rolling code
#endif
) {
        return;
    }

    uint8_t i;
    for (i=0;i<b->byteidx;i++)
      obuf[i]=b->data[i];

    if (7-b->bitidx != 0) {
      obuf[i]=b->data[i];
      i++;
    }
    *oby = i;
    if (b->byteidx == 7 || ( b->byteidx == 3 &&  b->bitidx == 3 )) {
      b->state = STATE_HE;
    } 
    *datatype = TYPE_IT;
    
  }
}


/*
 * Description in header
 */
uint8_t sync_intertechno(bucket_t *b, pulse_t *hightime, pulse_t *lowtime) 
{
   if(IS433MHZ && (b->state == STATE_IT || b->state == STATE_ITV3)) {
    if (*lowtime > TSCALE(15000)) { 
        return 1;
    } else if (*lowtime > TSCALE(2100) || *hightime == 0) {
        b->sync = 0;
    } 
    if (b->sync == 0) {
      if (*lowtime > TSCALE(2100) && *lowtime < TSCALE(3400)) { 
        b->state = STATE_ITV3;
        return 1;
      } else if (b->state == STATE_ITV3) {
        b->sync=1;
        b->zero.hightime = *hightime; 
		    b->zero.lowtime = *lowtime;
        b->one.hightime = *hightime;
	      b->one.lowtime = *hightime;
      } else {
        b->sync=1;
        OCR1A = 2200; // end of message
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
  if(IS433MHZ && (*hightime < TSCALE(600) && *hightime > TSCALE(140))) {
    if ((*lowtime < TSCALE(2750) && *lowtime > TSCALE(2350))) {
      b->state = STATE_ITV3;
      OCR1A = 3200; // end of message
#ifdef HAS_HOMEEASY
    } else if ((*lowtime < TSCALE(5250) && *lowtime > TSCALE(4750))) {
      b->state = STATE_ITV3;
      OCR1A = 2800; // end of message
    } else if ((*lowtime < TSCALE(9250) && *lowtime > TSCALE(9000))) {
      b->state = STATE_ITV3;
      OCR1A = 3200; // end of message
#endif
    } else if (*lowtime < TSCALE(15000) && *lowtime > TSCALE(6000)) {
      //DC('0');
      b->state = STATE_IT;
      OCR1A = 3200; // end of message
    } else {
      return 0;
    }
    
    TIMSK1 = _BV(OCIE1A);
    b->sync=0;
    
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
    b->one.hightime = makeavg(b->one.hightime,*hightime);
    b->zero.hightime = makeavg(b->one.hightime,*hightime);
    if(*lowtime > TDIFF + *hightime) {
      addbit(b, 1);
       b->one.lowtime = makeavg(b->one.lowtime,*lowtime);
    } else {
      addbit(b, 0);
      b->zero.lowtime = makeavg(b->zero.lowtime,*lowtime);
    }
  }
}

#endif

