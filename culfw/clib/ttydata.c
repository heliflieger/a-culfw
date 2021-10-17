#include <avr/pgmspace.h>               // for PSTR, PROGMEM, __LPM, etc
#include <stdint.h>                     // for uint8_t

#include "board.h"                      // for TTY_BUFSIZE
#include "display.h"                    // for display_channel, DC, DS_P, etc
#include "ttydata.h"
#include "rf_mode.h"
#include "hw_autodetect.h"
#ifdef ARM
#include <utility/trace.h>
#else
#define TRACE_DEBUG(...)      { }
#endif

void (*input_handle_func)(uint8_t channel);
void (*output_flush_func)(void);


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
#ifdef USE_HW_AUTODETECT
      if( !(((n == '*') && !has_CC(CC1101.instance+1)) || ((n == 'O') && !has_onewire())) )
#endif
      {
      DC(' ');
      DC(n);
      }
#ifdef USE_HW_AUTODETECT
    } else if((buf[0] == n ) && (n == 'O')) {
        if(has_onewire()) {
          fn(buf);
          return 1;
        } else {
          return 0;
        }
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

#ifdef HAS_MULTI_CC
      CC1101.instance = 0;
#endif

      cmdbuf[cmdlen] = 0;
      TRACE_DEBUG("TTYDATA received: %s\n\r", cmdbuf);
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

