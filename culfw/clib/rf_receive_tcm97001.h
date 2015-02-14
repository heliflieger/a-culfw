#ifndef _RF_RECEIVE_TCM97001_H
#define _RF_RECEIVE_TCM97001_H

#include <stdio.h>
#include "board.h"
#include "rf_receive_bucket.h"
#include "fband.h"


#define STATE_TCM97001 8
#define TYPE_TCM97001 's'


/*
 * Analyse TCM97001 packet.
 * Parameter:
 *  b        = the received bucket
 *  datatype = the datatype
 *  obuf     = the object buffer
 *  oby      = the oby count
 * The datatype are set to TYPE_TCM97001, if packet is correct.
 */
void analyze_tcm97001(bucket_t *b, uint8_t *datatype, uint8_t *obuf, uint8_t *oby);

/*
 * Sync the TCM97001 received packet
 * return 1 (true) if packet ist OK
 * return 0 (false) if packet is not TCM97001 
 */
uint8_t sync_tcm97001(bucket_t *b, pulse_t *hightime, pulse_t *lowtime);

/*
 * Check if received packat is an TCM97001 packet
 * hightime the received hightime
 * lowtime the received lowtime
 * return 1 (true) if packet ist OK
 * return 0 (false) if packet is not TCM97001 
 */
uint8_t is_tcm97001(bucket_t *b, pulse_t *hightime, pulse_t *lowtime);

/*
 * Add the the bit to the bucket if it is an TCM97001 bit.
 * hightime the received hightime
 * lowtime the received lowtime
 * return 1 (true) if packet ist OK
 * return 0 (false) if packet is not TCM97001 
 */
uint8_t addbit_tcm97001(bucket_t *b, pulse_t *hightime, pulse_t *lowtime);


#endif
