#ifndef __FREQUENCY_BAND_H_
#define __FREQUENCY_BAND_H_

#define MODE_UNKNOWN    0
#define MODE_433_MHZ    1
#define MODE_868_MHZ    2

extern uint8_t frequencyMode;

// function to init the frequency flag
void checkFrequency(void); 
// if 433 MHZ is enabled return true (0)
uint8_t is433MHz(void);
// if 868 MHZ is enabled return true (0)
uint8_t is868MHz(void);

#endif
