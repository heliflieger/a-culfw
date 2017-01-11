/* 
 * Copyright by B.Hempel
 * License: GPL v2
 */

#ifndef _RF_RECEIVE_ESA_H
#define _RF_RECEIVE_ESA_H

#include <stdint.h>                     // for uint8_t

#include "board.h"                      // for HAS_ESA
#include "rf_receive_bucket.h"          // for bucket_t

#ifdef HAS_ESA

#define TYPE_ESA     'S'

#define STATE_ESA     5

/*
 * Analyse ESA packet.
 * Parameter:
 *  b        = the received bucket
 *  datatype = the datatype
 *  obuf     = the object buffer
 *  oby      = the oby count
 * The datatype are set to TYPE_ESA, if packet is correct.
 */
void analyze_esa(bucket_t *b, uint8_t *datatype, uint8_t *obuf, uint8_t *oby);

#endif

#endif

