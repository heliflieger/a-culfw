/* 
 * Copyright by B.Hempel
 * License: GPL v2
 */

#include "rf_receive_oregon.h"
#include "display.h"

#ifdef HAS_OREGON3
/*
 * Reverse nibble for oregon
 */
uint8_t reverse_bitsinnibble(uint8_t in) {
  uint8_t ret=0;
  ret = (in&0x11)<<3;
  ret |= (in&0x22)<<1;
  ret |= (in&0x44)>>1;
  ret |= (in&0x88)>>3;
  return ret;
}

void analyze_oregon3(bucket_t *b, uint8_t *datatype, uint8_t *obuf, uint8_t *oby)
{
  if (IS433MHZ && *datatype == 0) {
    if (b->byteidx != 13 || b->state != STATE_OREGON3 || b->bitidx != 3)
      return;
    uint8_t sum=0;
    uint8_t tmp;
    uint8_t i;
    for (i=0;i<14;i++) {    
      tmp=reverse_bitsinnibble(b->data[i]);
      obuf[i]=tmp;
      if (i>=1 && i<11)
        sum+=(tmp&0xf)+(tmp>>4);
    }
    sum+=(obuf[11]>>4);
    obuf[13]=obuf[13]>>4;
    i++;    
    //nibble=1;
    if (sum != ((obuf[11]&0xf) | (obuf[12]&0xf0)))
      return;
    *oby = i;
    *datatype = TYPE_OREGON3;
  }
}

#endif



