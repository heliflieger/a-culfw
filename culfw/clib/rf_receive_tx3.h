/* 
 * Copyright by B.Hempel
 * License: GPL v2
 */

#ifndef _RF_RECEIVE_TX3_H
#define _RF_RECEIVE_TX3_H

#include <stdint.h>                     // for uint8_t

#include "rf_receive_bucket.h"          // for bucket_t


#define TYPE_TX3     't'


/*
 * Analyse TX packet.
 * Parameter:
 *  b        = the received bucket
 *  datatype = the datatype
 *  obuf     = the object buffer
 *  oby      = the oby count
 * The datatype are set to TYPE_TX3, if packet is correct.
 */
void analyze_tx3(bucket_t *b, uint8_t *datatype, uint8_t *obuf, uint8_t *oby);


#endif
