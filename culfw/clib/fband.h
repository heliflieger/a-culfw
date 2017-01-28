#ifndef __FREQUENCY_BAND_H_
#define __FREQUENCY_BAND_H_

#include <stdint.h>                     // for uint8_t
#include "board.h"

#define MODE_UNKNOWN    0
#define MODE_433_MHZ    1
#define MODE_868_MHZ    2

#ifdef HAS_MULTI_CC
#include "multi_CC.h"
#define IS433MHZ (multiCC.frequencyMode[multiCC.instance] == MODE_433_MHZ)
#define IS868MHZ (multiCC.frequencyMode[multiCC.instance] == MODE_868_MHZ)
#else
#define IS433MHZ (frequencyMode == MODE_433_MHZ)
#define IS868MHZ (frequencyMode == MODE_868_MHZ)
extern uint8_t frequencyMode;
#endif

// function to init the frequency flag
void checkFrequency(void); 


#endif
