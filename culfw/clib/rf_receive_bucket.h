#ifndef _RF_RECEIVE_BUCKET_H
#define _RF_RECEIVE_BUCKET_H

#include <avr/io.h>
#include <stdio.h>

/* public prototypes */
#ifdef HAS_ESA
#define MAXMSG 20               // ESA messages
#else
#define MAXMSG 12               // EMEM messages
#endif

#define TSCALE(x)  (x/16)      // Scaling time to enable 8bit arithmetic


#if defined(HAS_REVOLT) || defined (HAS_IT) || defined (HAS_TCM97001)
#define LONG_PULSE
typedef uint16_t pulse_t;
#else
typedef uint8_t pulse_t;
#endif

/*
 * Is defined in rf_receive.c
 */
extern void reset_input(void);

typedef struct {
  uint8_t hightime, lowtime;
} wave_t;

// One bucket to collect the "raw" bits
typedef struct {
  uint8_t state, byteidx, sync, bitidx; 
  uint8_t data[MAXMSG];         // contains parity and checksum, but no sync
  wave_t zero, one; 
} bucket_t;


/*
 * Add bit to bucket
 */
void addbit(bucket_t *b, uint8_t bit);

/*
 * Make avg for the received packet and return the value
 */
uint8_t makeavg(uint8_t i, uint8_t j);

#endif

