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

#ifndef _RF_RECEIVE_IT_H
#define _RF_RECEIVE_IT_H

#include <stdint.h>                     // for uint8_t

#include "board.h"                      // for HAS_IT
#include "rf_receive_bucket.h"          // for bucket_t

#ifdef HAS_IT

#define STATE_IT       7
#define STATE_ITV3     9
#define STATE_HE      10
#define TYPE_IT  	    'i'


/*
 * Analyse IT packet.
 * Parameter:
 *  b        = the received bucket
 *  datatype = the datatype
 *  obuf     = the object buffer
 *  oby      = the oby count
 * The datatype are set to TYPE_IT, if packet is correct.
 */
void analyze_intertechno(bucket_t *b, uint8_t *datatype, uint8_t *obuf, uint8_t *oby);


#endif
#endif
