/* 
 * Copyright by B.Hempel
 * License: GPL v2
 */

#include "rf_receive_tx3.h"

#include <stdint.h>                     // for uint8_t

#include "board.h"                      // for HAS_TX3

/*
 * Description in header
 */
void analyze_tx3(bucket_t *b, uint8_t *datatype, uint8_t *obuf, uint8_t *oby)
{
#ifdef HAS_TX3
  if (*datatype == 0) {
    input_t in;
    in.byte = 0;
    in.bit = 7;
    in.data = b->data;
    uint8_t i, n, crc = 0;
    if(b->byteidx != 4 || b->bitidx != 1)
      return;
    
    for(i = 0; i < 4; i++) {
      if(i == 0) {
        n = 0x80 | getbits(&in, 7, 1);
      } else {
        n = getbits(&in, 8, 1);
      }
      crc = crc + (n>>4) + (n&0xf);
      obuf[i] = n;
    }
    obuf[i] = getbits(&in, 7, 1) << 1;
    crc = (crc + (obuf[i]>>4)) & 0xF;
    i++;
    *oby = i;
    if((crc >> 4) != 0 || (obuf[0]>>4) != 0xA) {
      // CRC failed
      *datatype = TYPE_TX3;
      return;
    }
    *datatype = TYPE_TX3;
  }
#endif
}



