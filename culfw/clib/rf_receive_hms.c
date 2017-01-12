/* 
 * Copyright by B.Hempel
 * License: GPL v2
 */

#include "rf_receive_hms.h"

#include <stdint.h>                     // for uint8_t

#include "fband.h"                      // for IS868MHZ
#include "util/parity.h"                // for parity_even_bit

#ifdef HAS_HMS
/*
 * Description in header
 */
void analyze_hms(bucket_t *b, uint8_t *datatype, uint8_t *obuf, uint8_t *oby)
{
  if (IS868MHZ && *datatype == 0 && b->state == STATE_HMS) {
    input_t in;
    in.byte = 0;
    in.bit = 7;
    in.data = b->data;

    if(b->byteidx*8 + (7-b->bitidx) < 69) 
      return; // failed

    uint8_t crc = 0, i = 0;
    for(i = 0; i < 6; i++) {
      obuf[i] = getbits(&in, 8, 0);
      if(parity_even_bit(obuf[i]) != getbit( &in ))
        return; // failed
      if(getbit(&in))
        return; // failed
      crc = crc ^ obuf[i];
    }
    *oby = i;
    // Read crc
    uint8_t CRC = getbits(&in, 8, 0);
    if(parity_even_bit(CRC) != getbit(&in))
      return; // failed
    if(crc!=CRC)
      return; // failes
     
    *datatype = TYPE_HMS;
    return; // OK
  }
}

#endif


