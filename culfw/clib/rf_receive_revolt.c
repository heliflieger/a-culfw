/* 
 * Copyright by B.Hempel
 * License: GPL v2
 */

#include "rf_receive_revolt.h"
#include "display.h"

#ifdef HAS_REVOLT
/*
 * Description in header
 */
void analyze_revolt(bucket_t *b, uint8_t *datatype, uint8_t *obuf, uint8_t *oby)
{
  if (IS433MHZ && *datatype == 0) {
    uint8_t sum=0;
    if (b->byteidx != 12 || b->state != STATE_REVOLT || b->bitidx != 0)
      return;

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

#endif


