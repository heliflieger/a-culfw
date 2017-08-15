#include "fband.h"

#include <stdint.h>                     // for uint8_t, uint16_t, uint32_t

#include "fncollection.h"               // for erb
#include "stringfunc.h"                 // for fromhex

uint8_t frequencyMode = MODE_UNKNOWN;



/*
 * Read eeprom
 */
uint8_t readEEpromValue(char *rbyte) {
  uint8_t hb[2], d;
  uint16_t addr;
  hb[0] = hb[1] = 0;
  d = fromhex(rbyte, hb, 2);
  if(d == 2)
    addr = (hb[0] << 8) | hb[1];
  else
    addr = hb[0];
  d = erb((uint8_t *)addr);
  return d;
}

/*
 * Check the frequency bits and set the frequencyMode value
 */
void checkFrequency(void) {
  uint32_t value_0D; 
  uint16_t value_0E;
  uint8_t  value_0F;
#ifdef HAS_MULTI_CC
#if NUM_SLOWRF > 1
  if (CC1101.instance == 1) {
    value_0D = erb((uint8_t *)EE_CC1100_CFG1 + 0x0D);
    value_0E = erb((uint8_t *)EE_CC1100_CFG1 + 0x0E);
    value_0F = erb((uint8_t *)EE_CC1100_CFG1 + 0x0F);
  } else
#endif
#if NUM_SLOWRF > 2
    if (CC1101.instance == 2) {
    value_0D = erb((uint8_t *)EE_CC1100_CFG2 + 0x0D);
    value_0E = erb((uint8_t *)EE_CC1100_CFG2 + 0x0E);
    value_0F = erb((uint8_t *)EE_CC1100_CFG2 + 0x0F);
  } else
#endif
#endif
  {
  value_0D = readEEpromValue("0F");
  value_0E = readEEpromValue("10");
  value_0F = readEEpromValue("11");
  }
  uint16_t frequency = 26*(value_0D*256*256+value_0E*256+value_0F)/65536;

#ifdef USE_RF_MODE
  if (frequency > 500) {
    CC1101.frequencyMode[CC1101.instance] = MODE_868_MHZ;
  } else {
    CC1101.frequencyMode[CC1101.instance] = MODE_433_MHZ;
  }
#else
  if (frequency > 500) {
    frequencyMode = MODE_868_MHZ;
  } else {
    frequencyMode = MODE_433_MHZ;
  }
#endif
}


