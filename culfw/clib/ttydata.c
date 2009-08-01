#include "display.h"
#include "ttydata.h"
#include "cdc.h"

uint8_t
callfn(char *buf)
{
  for(uint8_t idx = 0; fntab[idx].name; idx++) {
    if(buf[0] == fntab[idx].name) {
      fntab[idx].fn(buf);
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
  uint8_t idx, ucCommand;
    
  while(USB_Rx_Buffer->nbytes) {

    ucCommand = rb_get(USB_Rx_Buffer);
    //DC(ucCommand);                       // echo

    if(ucCommand == '\n' || ucCommand == '\r') {

      if(!cmdlen)       // empty return
        continue;

      cmdbuf[cmdlen] = 0;
      if(!callfn(cmdbuf)) {
        DS_P(PSTR("? ("));
        DC(cmdbuf[0]);
        DS_P(PSTR(" is unknown) Use one of"));

        for(idx = 0; fntab[idx].name; idx++) {
          DC(' ');
          DC(fntab[idx].name);
        }
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
