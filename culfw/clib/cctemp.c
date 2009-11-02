#include <avr/io.h>
#include "board.h"
#include "cc1100.h"
#include "delay.h"
#include "stringfunc.h"
#include "rf_receive.h"
#include "adcw.h"
#include "display.h"

// CC1101.pdf, Page 15, 4.7:
// -40: 0.651V
//   0: 0.747V
//  40: 0.847V
//  80: 0.945C
// Measured:
//  23.6 == 400
//   3.2 == 375
// We read 1024 for 2.56V and 0 for 0V -> we have a resolution of 1°C

void
cctemp_func(char *in)
{
  uint16_t bv;
  uint8_t hb[1];
  uint8_t l = fromhex(in+1,hb,2);

  // Its assumed we are in RX state, not going into IDLE
  CLEAR_BIT( CC1100_OUT_DDR, CC1100_OUT_PIN);
  cc1100_writeReg(CC1100_IOCFG0, 0x80); // Page 64

  bv = get_adcw(CCTEMP_MUX);

  if(l >= 1 && hb[0]) {
    DU(bv, 4);

  } else {
    DU(bv-372, 4);

  }
  DNL();

  cc1100_writeReg(CC1100_IOCFG0, 0x2d); 
  SET_BIT( CC1100_OUT_DDR, CC1100_OUT_PIN);
  
 }
