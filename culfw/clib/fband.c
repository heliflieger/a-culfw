#include "board.h"
#include "display.h"

#include "fncollection.h"
#include "cc1100.h"

#include "fband.h"

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
  value_0D = readEEpromValue("0F");
  value_0E = readEEpromValue("10");
  value_0F = readEEpromValue("11");
  uint16_t frequency = 26*(value_0D*256*256+value_0E*256+value_0F)/65536;

  if (frequency > 500) {
    frequencyMode = MODE_868_MHZ;
  } else {
    frequencyMode = MODE_433_MHZ;
  }
}


