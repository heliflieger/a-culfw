#ifndef _RF_RECEIVE_BUCKET_H
#define _RF_RECEIVE_BUCKET_H

#include <avr/io.h>
#include <stdio.h>
#include "board.h"

/* public prototypes */
#ifdef HAS_ESA
#define MAXMSG 20               // ESA messages
#else
#define MAXMSG 12               // EMEM messages
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

/*
 * The input struct
 */
typedef struct  {
  uint8_t *data;
  uint8_t byte, bit;
} input_t;

/*
 * This struct has the bits for receive check
 */
struct {
   uint8_t isrep:1; // 1 Bit for is repeated
   uint8_t isnotrep:1; // 1 Bit for is repeated value
   uint8_t packageOK:1; // Received packet is ok
   // 5 bits free
} packetCheckValues;


/*
 * Is defined in rf_receive.c
 */
extern void reset_input(void);

typedef struct {
  pulse_t hightime, lowtime;
} wave_t;
#ifdef DEBUG_SYNC
typedef struct {
  uint16_t hightime, lowtime;
} wave_t16;
#endif

// One bucket to collect the "raw" bits
typedef struct {
  uint8_t state, byteidx, sync, bitidx; 
  uint8_t data[MAXMSG];         // contains parity and checksum, but no sync
  wave_t zero, one; 
#ifdef DEBUG_SYNC
  wave_t16 syncbit;
#endif
} bucket_t;


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

