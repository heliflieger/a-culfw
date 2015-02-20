/* 
 * Copyright by B.Hempel
 * License: GPL v2
 */

#include "rf_receive_bucket.h"

uint8_t makeavg(uint8_t i, uint8_t j)
{
  return (i+i+i+j)/4;
}

/*
 * Description in header
 */
void addbit(bucket_t *b, uint8_t bit)
{
  if(b->byteidx>=sizeof(b->data)){
    reset_input();
    return;
  }
  if(bit)
    b->data[b->byteidx] |= _BV(b->bitidx);

  if(b->bitidx-- == 0) {           // next byte
    b->bitidx = 7;
    b->data[++b->byteidx] = 0;
  }
}



