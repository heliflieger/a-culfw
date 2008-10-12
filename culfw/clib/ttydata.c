#include "display.h"
#include "ttydata.h"

void
analyze_ttydata(uint8_t ucCommand)
{
  static char cmdbuf[33];
  static uint8_t cmdlen;
  uint8_t idx;
    
  //DC(ucCommand);                       // echo

  if(ucCommand == '\n' || ucCommand == '\r') {
    cmdbuf[cmdlen] = 0;

    for(idx = 0; fntab[idx].name; idx++) {
      if(cmdbuf[0] == fntab[idx].name) {
        fntab[idx].fn(cmdbuf);
        break;
      }
    }
    if(!fntab[idx].name && cmdlen) {
      DC('?'); DC('?');
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
