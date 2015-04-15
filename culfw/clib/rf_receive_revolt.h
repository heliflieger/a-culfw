/* 
 * Copyright by B.Hempel
 * License: GPL v2
 */

#ifndef _RF_RECEIVE_REVOLT_H
#define _RF_RECEIVE_REVOLT_H


#include <stdio.h>
#include "board.h"
#include "rf_receive_bucket.h"
#include "fband.h"

#ifdef HAS_REVOLT

#define TYPE_REVOLT	 'r'

#define STATE_REVOLT  6

/*
 * Analyse REVOLT packet.
 * Parameter:
 *  b        = the received bucket
 *  datatype = the datatype
 *  obuf     = the object buffer
 *  oby      = the oby count
 * The datatype are set to TYPE_REVOLT, if packet is correct.
 */
void analyze_revolt(bucket_t *b, uint8_t *datatype, uint8_t *obuf, uint8_t *oby);

#endif
#endif
