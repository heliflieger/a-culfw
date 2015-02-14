/* 
 * Copyright by B.Hempel
 * License: GPL v2
 */

#include "rf_receive_tcm97001.h"
#include "display.h"

/*
 * Description in header
 */
void analyze_tcm97001(bucket_t *b, uint8_t *datatype, uint8_t *obuf, uint8_t *oby)
{
#ifdef HAS_TCM97001
  if (IS433MHZ && *datatype == 0) {
    
    if (b->byteidx != 3 || b->bitidx != 7 || b->state != STATE_TCM97001) {
		  return;
    }
    uint8_t i;
    for (i=0;i<b->byteidx;i++) {
      obuf[i]=b->data[i];
    }
    *oby = i;
    *datatype = TYPE_TCM97001;
  }
#endif
}

/*
 * Description in header
 */
uint8_t sync_tcm97001(bucket_t *b, pulse_t *hightime, pulse_t *lowtime) 
{
#ifdef HAS_TCM97001
  if (IS433MHZ && b->state == STATE_TCM97001 && b->sync == 0) {
	  b->sync=1;
		b->zero.hightime = *hightime;
		b->one.hightime = *hightime;

		if (*lowtime < 187) { // < 3000
			b->zero.lowtime = *lowtime;
			b->one.lowtime = b->zero.lowtime*2;
		} else {
			b->zero.lowtime = *lowtime;
			b->one.lowtime = b->zero.lowtime/2;
		}
    return 1;
	}
#endif
  return 0;
}

/*
 * Description in header
 */
uint8_t is_tcm97001(bucket_t *b, pulse_t *hightime, pulse_t *lowtime) 
{
#ifdef HAS_TCM97001
  if (IS433MHZ && (*hightime < TSCALE(530) && *hightime > TSCALE(420)) &&
				   (*lowtime  < TSCALE(9000) && *lowtime > TSCALE(8500)) ) {
		  OCR1A = 4600L;
			TIMSK1 = _BV(OCIE1A);
			b->sync=0;
      
			b->state = STATE_TCM97001;
			b->byteidx = 0;
			b->bitidx  = 7;
			b->data[0] = 0;  
		  return 1;
	}
#endif
  return 0;
}

/*
 * Description in header
 */
uint8_t addbit_tcm97001(bucket_t *b, pulse_t *hightime, pulse_t *lowtime) 
{
#ifdef HAS_TCM97001
  if (IS433MHZ && b->state==STATE_TCM97001) {
    if (*lowtime > 110 && *lowtime < 140) {
      addbit(b,0);
      b->zero.hightime = makeavg(b->zero.hightime, *hightime);
      b->zero.lowtime  = makeavg(b->zero.lowtime,  *lowtime);
    } else if (*lowtime > 230 && *lowtime < 270) {
      addbit(b,1);
      b->one.hightime = makeavg(b->one.hightime, *hightime);
      b->one.lowtime  = makeavg(b->one.lowtime,  *lowtime);
    }
    return 1;
  }
#endif
  return 0;
}


