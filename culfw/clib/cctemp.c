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

static uint8_t
raw2cels(uint16_t bv)
{
  static uint8_t tmp[] = { 0,109,136,141,143,152,164,179,196,214,240 };
  uint8_t s1, s2;

  if(bv < 761) bv = 761;
  bv -= 760;
  if(bv > 240) bv = 240;

  uint8_t b = (uint8_t)bv;
  for(s1 = 1; s1 < sizeof(tmp); s1++)
    if(b <= tmp[s1])
      break;
  
  s2 = s1-1;
  return 10*s2+(10*(uint16_t)(b-tmp[s2]))/(uint16_t)(tmp[s1]-tmp[s2]);
}

void
cctemp_func(char *in)
{
  uint16_t bv;
  uint8_t hb[1];
  uint8_t l = fromhex(in+1,hb,2);

  // Its assumed we are in RX state, not going into IDLE
  
//  ccStrobe(CC1100_SIDLE);  
//  my_delay_ms(1);                       // Needed if calibrating on RX/TX->SIDLE

  CLEAR_BIT( CC1100_OUT_DDR, CC1100_OUT_PIN);
  CLEAR_BIT( CC1100_OUT_PORT, CC1100_OUT_PIN);
  
  cc1100_writeReg(CC1100_IOCFG0, 0x80); // Page 64
//  cc1100_writeReg(CC1100_PTEST,  0xBF); // Page 15 & 84

//  ccStrobe(CC1100_SIDLE);

  bv = get_adcw(CCTEMP_MUX);

  if(l >= 1 && hb[0]) {
    DU(bv, 4);

  } else {
    DU(raw2cels(bv), 4);

  }
  DNL();

//  cc1100_writeReg(CC1100_PTEST,  0x7F);
//  cc1100_writeReg(CC1100_IOCFG0, 0x2d); 
  
//  set_txrestore();

  SET_BIT( CC1100_OUT_DDR, CC1100_OUT_PIN);
  
 }
