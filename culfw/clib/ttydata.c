#include "display.h"
#include "ttydata.h"
#include <avr/pgmspace.h>

void (*input_handle_func)(uint8_t channel);


rb_t TTY_Tx_Buffer;
rb_t TTY_Rx_Buffer;
static char cmdbuf[TTY_BUFSIZE+1];

extern const PROGMEM t_fntab fntab[];
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
analyze_ttydata(uint8_t channel)
{
  static uint8_t cmdlen;
  uint8_t ucCommand;
  uint8_t odc = display_channel;
  display_channel = channel;
    
  while(TTY_Rx_Buffer.nbytes) {

    ucCommand = rb_get(&TTY_Rx_Buffer);

#ifdef RPI_TTY_FIX
    // eat RPi rubbish
    if (ucCommand == 0xff)
      continue;
#endif

    if(ucCommand == '\n' || ucCommand == '\r') {

      if(!cmdlen)       // empty return
        continue;

      cmdbuf[cmdlen] = 0;
      if(!callfn(cmdbuf)) {
        DS_P(PSTR("? ("));
        display_string(cmdbuf);
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
  display_channel = odc;
}

