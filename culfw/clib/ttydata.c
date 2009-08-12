#include "display.h"
#include "ttydata.h"
#include "cdc.h"
#include <avr/pgmspace.h>


extern PROGMEM t_fntab fntab[];
uint8_t
callfn(char *buf)
{
  for(uint8_t idx = 0; ; idx++) {
    uint8_t n = __LPM(&fntab[idx].name);
    void (*fn)(char *) = (void (*)(char *))__LPM_word(&fntab[idx].fn);
    if(!n)
      break;
    if(buf == 0) {
      DC(' ');
      DC(n);
    } else if(buf[0] == n) {
      fn(buf);
      return 1;
    }
  }
  return 0;
}

void
analyze_ttydata()
{
  static char cmdbuf[33];
  static uint8_t cmdlen;
  uint8_t ucCommand;
    
  while(USB_Rx_Buffer->nbytes) {

    ucCommand = rb_get(USB_Rx_Buffer);

    if(ucCommand == '\n' || ucCommand == '\r') {

      if(!cmdlen)       // empty return
        continue;

      cmdbuf[cmdlen] = 0;
      if(!callfn(cmdbuf)) {
        DS_P(PSTR("? ("));
        DC(cmdbuf[0]);
        DS_P(PSTR(" is unknown) Use one of"));
        callfn(0);
        DNL();
      }
      cmdlen = 0;

    } else {
      if(cmdlen < sizeof(cmdbuf)-1)
        cmdbuf[cmdlen++] = ucCommand;
    }
  }
}

void
tty_init(void)
{
  usbinfunc = analyze_ttydata;
}
