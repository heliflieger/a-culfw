#include "board.h"
#include "display.h"
#include "ringbuffer.h"
#include "cdc.h"
#include "led.h"
#include "delay.h"
#include "pcf8833.h"
#include "ttydata.h"            // callfn
#include "fht.h"                // fht_hc
#include "log.h"

#ifdef HAS_ETHERNET
#include "tcplink.h"
#endif

uint8_t output_enabled;

//////////////////////////////////////////////////
// Display routines
void
display_char(char data)
{

#ifdef HAS_ETHERNET
  tcp_putchar( data );
#endif

#ifdef HAS_USB
  if(USB_IsConnected && (output_enabled & OUTPUT_USB)) {
    if(USB_Tx_Buffer->nbytes >= USB_Tx_Buffer->size)
      CDC_Task();
    rb_put(USB_Tx_Buffer, data);
    if(data == '\n')
      CDC_Task();
  }
#endif


#ifdef HAS_LCD
  if(output_enabled & OUTPUT_LCD) {
    static uint8_t buf[TITLE_LINECHARS+1];
    static uint8_t off = 0, cmdmode = 0;
    if(data == '\r')
      return;

    if(data == '\n') {
      buf[off] = 0;
      off = 0;
      if(cmdmode) {
        callfn((char *)buf);
      } else 
        lcd_putline(0, (char*)buf);
      cmdmode = 0;
    } else {
      // 
      if(off < TITLE_LINECHARS)   // or cmd: up to 12Byte: F12346448616c6c6f
        buf[off++] = data;

      if(cmdmode && cmdmode++ == 2) {
        off -= 2;
        fromhex((char *)buf+off, buf+off, 1);   // replace the hexnumber
        off++;
        cmdmode = 1;
      }
    }

    // Check if its a message for us: F<HC>..., and set cmdmode
    if(!cmdmode && off == 5 && buf[0] == 'F') {
      uint8_t hb[2];
      fromhex((char*)buf+1, hb, 2);
      if(hb[0] == fht_hc0 && hb[1] == fht_hc1) {
        cmdmode = 1;
        off = 0;
      }
    }
  }
#endif

#ifdef HAS_FS
  if(output_enabled & OUTPUT_LOG) {
    static uint8_t buf[LOG_NETTOLINELEN+1];
    static uint8_t off = 0;
    if(data == '\r')
      return;

    if(data == '\n') {
      buf[off] = 0;
      off = 0;
      Log((char*)buf);
    } else {
      if(off < LOG_NETTOLINELEN)
        buf[off++] = data;
    }
  }
#endif
}

void
display_string(char *s)
{
  while(*s)
    display_char(*s++);
}

void
display_string_P(prog_char *s)
{
  uint8_t c;
  while((c = __LPM(s))) {
    display_char(c);
    s++;
  }
}

void
display_nl()
{
  display_char('\r');
  display_char('\n');
}

void
display_udec(uint16_t d, int8_t pad, uint8_t padc)
{
  char buf[6];
  int8_t i=6;

  buf[--i] = 0;
  do {
    buf[--i] = d%10 + '0';
    d /= 10;
    pad--;
  } while(d && i);

  while(--pad >= 0 && i > 0)
    buf[--i] = padc;
  DS(buf+i);
}

void
display_hex(uint16_t h, int8_t pad, uint8_t padc)
{
  char buf[5];
  int8_t i=5;

  buf[--i] = 0;
  do {
    uint8_t m = h%16;
    buf[--i] = (m < 10 ? '0'+m : 'A'+m-10);
    h /= 16;
    pad--;
  } while(h);

  while(--pad >= 0 && i > 0)
    buf[--i] = padc;
  DS(buf+i);
}

void
display_hex2(uint8_t h)
{
  display_hex(h, 2, '0');
}
