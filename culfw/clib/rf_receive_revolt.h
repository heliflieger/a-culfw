/* 
 * a-culfw
 * Copyright (C) 2015 B. Hempel
 *
 * This program is free software; you can redistribute it and/or modify it under  
 * the terms of the GNU General Public License as published by the Free Software  
 * Foundation; either version 2 of the License, or (at your option) any later  
 * version.
 * 
 * This program is distributed in the hope that it will be useful, but  
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY  
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for  
 * more details.
 * 
 * You should have received a copy of the GNU General Public License along with  
 * this program; if not, write to the  
 * Free Software Foundation, Inc.,  
 * 51 Franklin St, Fifth Floor, Boston, MA 02110, USA
 */

#ifndef _RF_RECEIVE_REVOLT_H
#define _RF_RECEIVE_REVOLT_H


#include <stdbool.h>                    // for bool
#include <stdint.h>                     // for uint8_t

#include "board.h"                      // for HAS_REVOLT
#include "rf_receive_bucket.h"          // for pulse_t, bucket_t

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

/*
 * Check if received packat is a revolt packet
 * hightime the received hightime
 * lowtime the received lowtime
 * return true if packet ist OK
 * return false if packet is not revolt 
 */
bool is_revolt(bucket_t *b, pulse_t *hightime, pulse_t *lowtime);

/*
 * Add the the bit to the bucket if it is a revolt bit.
 * hightime the received hightime
 * lowtime the received lowtime
 */
void addbit_revolt(bucket_t *b, pulse_t *hightime, pulse_t *lowtime);

#endif
#endif
