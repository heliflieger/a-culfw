#include "display.h"
#include "ttydata.h"
#include "cdc.h"

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

      for(idx = 0; fntab[idx].name; idx++) {
        if(cmdbuf[0] == fntab[idx].name) {
          fntab[idx].fn(cmdbuf);
          break;
        }
      }
      if(!fntab[idx].name) {
        DC('?');
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
