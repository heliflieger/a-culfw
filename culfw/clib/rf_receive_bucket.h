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
 *
 * 
 */

#ifndef _RF_RECEIVE_BUCKET_H
#define _RF_RECEIVE_BUCKET_H

#include <stdbool.h>                    // for bool
#include <stdint.h>                     // for uint8_t, uint16_t

#include "board.h"                      // for HAS_IT, HAS_REVOLT, etc
#include "rf_receive.h"

#define STATE_SYNC_PACKAGE      11
//#define STATE_BRESSER 12
#define STATE_MC                13
#define TYPE_SYNC_PACKAGE       '?'
#define TYPE_MC                 'o'

/* public prototypes */
#if defined(HAS_ESA) || defined (HAS_MANCHESTER) || defined(HAS_REVOLT)
#define MAXMSG 50               // ESA messages
//#define MAXMSGVALS (MAXMSG*8)
#else
#define MAXMSG 12               // EMEM messages
//#define MAXMSGVALS (MAXMSG*8)
#endif

#define TSCALE(x)  (x/16)      // Scaling time to enable 8bit arithmetic


#define TDIFF      TSCALE(200) // tolerated diff to previous/avg high/low/total

#define STATE_RESET   0

#define SILENCE    4000        // End of message


#if defined(HAS_REVOLT) || defined (HAS_IT) || defined (HAS_TCM97001)
#define LONG_PULSE
typedef uint16_t pulse_t;
#else
typedef uint8_t pulse_t;
#endif

typedef uint16_t pulseVal_t;

/*
 * The input struct
 */
typedef struct  {
  uint8_t *data;
  uint8_t *dataVals;
  uint8_t byte, bit;
} input_t;

/*
 * This struct has the bits for receive check
 */
typedef struct {
   uint8_t isrep:1; // 1 Bit for is repeated
   uint8_t isnotrep:1; // 1 Bit for is repeated value
   uint8_t packageOK:1; // Received packet is ok
   // 5 bits free
} packetCheckValues_t;
extern packetCheckValues_t packetCheckValues[NUM_SLOWRF];


/*
 * Is defined in rf_receive.c
 */
extern void reset_input(void);

typedef struct {
  pulse_t hightime, lowtime;
} wave_t;
typedef struct {
  uint16_t hightime, lowtime;
} wave_t16;

// One bucket to collect the "raw" bits
typedef struct {
  uint8_t state, byteidx, sync, bitidx; 
  uint8_t data[MAXMSG];         // contains parity and checksum, but no sync
//  pulseVal_t dataVals[MAXMSGVALS];
  uint8_t valCount;
  wave_t zero, one, two; 
#if defined(HAS_IT) || defined(HAS_TCM97001) 
  wave_t16 syncbit;
#endif
  uint16_t clockTime;
} bucket_t;


/*
 * Copy the data from bucket to receive buffer
 */
void copyData(uint8_t byteidx, uint8_t bitidx, uint8_t *data, uint8_t *obuf, uint8_t *oby, bool reverseBits);

/*
 * Add timing value to bucket
 */
void addTimeValBit(bucket_t *b, pulseVal_t valueh, pulseVal_t valuel);

/*
 * Add bit to bucket
 */
void addbit(bucket_t *b, uint8_t bit);

/*
 * Get bit
 */
uint8_t getbit(input_t *in);

/*
 * Get bits
 */
uint8_t getbits(input_t* in, uint8_t nbits, uint8_t msb);

/*
 * Make avg for the received packet and return the value
 */
uint8_t makeavg(uint8_t i, uint8_t j);


#endif

