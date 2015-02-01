#ifndef __FREQUENCY_BAND_H_
#define __FREQUENCY_BAND_H_

#define MODE_UNKNOWN    0
#define MODE_433_MHZ    1
#define MODE_868_MHZ    2

#define IS433MHZ (frequencyMode == MODE_433_MHZ)
#define IS868MHZ (frequencyMode == MODE_868_MHZ)

extern uint8_t frequencyMode;

// function to init the frequency flag
void checkFrequency(void); 


#endif
