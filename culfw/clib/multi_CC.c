#include "board.h"

#ifdef HAS_MULTI_CC

#include <stdint.h>

#include "multi_CC.h"
#include "display.h"
#include "ttydata.h"
#include "rf_mode.h"
#include "hw_autodetect.h"

extern const t_fntab fntab[];
extern const t_fntab fntab1[];
extern const t_fntab fntab2[];
extern const t_fntab fntab3[];

const t_fntab* fntabs[] =  {
    fntab,
    fntab1,
#if HAS_MULTI_CC > 2
    fntab2,
#endif
#if HAS_MULTI_CC > 3
    fntab3,
#endif
};

void multiCC_prefix(void) {
  for(uint8_t x=0; x<CC1101.instance; x++) {
    DC( '*' );
  }
}

uint8_t callfn2(char *buf)
{
  for(uint8_t idx = 0; ; idx++) {
    uint8_t n = fntabs[ CC1101.instance][idx].name;
    void (*fn)(char *) = (void (*)(char *))fntabs[ CC1101.instance][idx].fn;
    if(!n)
      break;
    if(buf == 0) {
#ifdef USE_HW_AUTODETECT
      if((n != '*') || has_CC(CC1101.instance+1) )
#endif
      {
        DC(' ');
        DC(n);
      }
#ifdef USE_HW_AUTODETECT
    } else if((buf[0] == n ) && (n == '*')) {
      if(has_CC(CC1101.instance+1)) {
        fn(buf);
        return 1;
      } else {
        return 0;
      }
#endif
    } else if(buf[0] == n) {
      fn(buf);
      return 1;
    }
  }
  return 0;
}

void multiCC_func(char *in) {

  in++;
  CC1101.instance++;

  if(!callfn2(in)) {
    multiCC_prefix();
    DS_P("? (");
    display_string(in);
    DS_P(" is unknown) Use one of");
    callfn2(0);
    DNL();
  }

  in--;
  CC1101.instance--;

}

#endif
