/* 
 * Copyright by B.Hempel
 * License: GPL v2
 */

#include "rf_receive_esa.h"
#include "display.h"

/*
 * Description in header
 */
void analyze_esa(bucket_t *b, uint8_t *datatype, uint8_t *obuf, uint8_t *oby)
{
#ifdef HAS_ESA
  if (IS868MHZ && *datatype == 0) {
    input_t in;
    in.byte = 0;
    in.bit = 7;
    in.data = b->data;

    if (b->state != STATE_ESA)
         return;

    if( (b->byteidx*8 + (7-b->bitidx)) != 144 )
         return;

    uint8_t salt = 0x89;
    uint16_t crc = 0xf00f;
    uint8_t i;
    for (i = 0; i < 15; i++) {
    
         uint8_t byte = getbits(&in, 8, 1);
       
         crc += byte;
      
         obuf[i] = byte ^ salt;
         salt = byte + 0x24;
         
    }
    
    obuf[i] = getbits(&in, 8, 1);
    crc += obuf[i];
    obuf[i++] ^= 0xff;
    *oby = i;
    crc -= (getbits(&in, 8, 1)<<8);
    crc -= getbits(&in, 8, 1);

    if (crc) 
         return; //failed

    *datatype = TYPE_ESA;
  }
#endif
}



