/*  
* Digital AC Dimmer  
*  
* Copyright (c) 2005-2006 Chris Fallin <chris@cfallin.org>  
* Placed under the modified BSD license  
*  
* dmx.c: DMX-512 transmitter module  
*/   

#include "board.h"   

#ifdef HAS_DMX
       
#include "dmx.h"   
#include "stringfunc.h"   
#include "display.h"   
#include <string.h>
#include <avr/io.h>   
#include <avr/interrupt.h>   
       
// PD2 (RXD1) and PD3 (TXD1) are connected to 75176's RXD and TXD; PA3 is   
// connected to REN/TXEN (high = transmit)   
       
static char levels[DMX_CHANNELS];   
static int channel_count;   
       
// the channel to be transmitted when the UART is ready for the next char;   
// a value of -1 indicates the start code   
// additionally, a value of -3 indicates that the break is currently being   
// transmitted, and -2 indicates that the mark-after-break is being   
// transmitted   

static int cur_channel;   
       
void dmx_initialize(int n_chan)   
{   
      channel_count = n_chan;   
      if(channel_count > DMX_CHANNELS)   
        channel_count = DMX_CHANNELS;   
       
      memset( levels, 0, sizeof( levels ));

      // initialize I/O pins   
      DDRA |= _BV( 3 ); PORTA |= _BV( 3 ); // PA3 = 1: 75176 always in transmit mode   
      DDRD |= _BV( 3 ); // PD3 == TX == out
       
      // initialize the USART   
      UBRR1H = 0;   
      UBRR1L = 1; // gives 250kbps for a 8MHz system clock   
      UCSR1A = 0x00;   
      UCSR1B = 0; // _BV( UDRIE1 ); // 0xd8; // everything enabled except for UDR Empty interrupt   
      UCSR1C = _BV(UCSZ10) | _BV(UCSZ11) | _BV(USBS1); // 8 data bits, 2 stop bits   
       
      // intialize Timer 3   
      TCCR3A = 0x00;   
      TCCR3B = _BV( CS31 ); // timer 3: CPU clock 8Mhz / 8 (1 tick = 1 us)   
}   
       
void dmx_set_level(int channel, char level)   
{   
      if(channel >= channel_count)   
        return;   
       
      levels[channel] = level;   
}   
       
char dmx_get_level(int channel)
{
      if(channel >= channel_count)
        return 0;

      return levels[channel];
}


void dmx_start()   
{   
      // start it off with the break   
      cur_channel = -3;   
       
      // disable the transmitter and set the pin low   
      UCSR1B = 0;   
      PORTD &= ~_BV( 3 );
       
      // start the timer   
      OCR3A = 87; // 88 us, or 22 bit times   
      TCNT3 = 0; // start the timer   
      TIMSK3 |= _BV(OCIE3A); // enable the int   
}   
       
ISR(USART1_UDRE_vect)   
{   
      // sanity check   
      if(cur_channel < 0)   
        return;   
       
      // if we've reached the end of our data, start the whole thing over again   
      if(cur_channel >= channel_count)   
      {   
        UCSR1B = 0; // disable INT and USART
#ifdef DMX_RESTING
        // start the timer   
	cur_channel = -4;
        OCR3A = 44 * (513-channel_count);
        TCNT3 = 0; // start the timer   
        TIMSK3 |= _BV(OCIE3A); // enable the int   
#else
        dmx_start();   
#endif
        return;   
      }   
       
      UDR1 = levels[cur_channel];   
      cur_channel++;   
}   
       
ISR(USART1_RX_vect)   
{   
      char rx_byte;   
       
      // discard the received byte and return   
      rx_byte = UDR1;   
}   
       
ISR(TIMER3_COMPA_vect)   
{   
      switch (cur_channel) {   

        case -4:
          dmx_start();
          break;

        case -3:
          TCNT3 = 0;   

          // we were transmitting the break; this interrupt ends it   
       
          // re-enable the transmitter   
          UCSR1B |= _BV( TXEN1 );   
       
          // now we move on to the mark-after-break: 8 us of idle   
       
          cur_channel = -2;   
          OCR3A = 7; // 8 us, or 2 bit times   
          break;

        case -2:   
          // disable this int
          TIMSK3 = 0;

          // we were in the mark-after-brea; this interrupt ends it   
       
          // transmit the start code (always zero in our implementation: no custom   
          // non-dimmer data packets)   
          UDR1 = 0;   
          UCSR1B |= _BV( UDRIE1 );
       
          // when that's done, we start with the channel data   
          cur_channel = 0;   
  
	  break;
       
     }

     TIFR3 |= _BV( OCF3A );

}   

void dmx_func(char *in) {
  uint8_t hb[4], d = 0;

  if(in[1] == 'r') {                // print latest record
    memset( hb, 0, sizeof(hb) );
    d = fromhex(in+2, hb, 1);
    if(d) {
       DH2( dmx_get_level( hb[0] ));
       DNL();
    }

  } else if(in[1] == 'w') {

    memset( hb, 0, sizeof(hb) );
    d = fromhex(in+2, hb, 2);
    if(d == 2) {
       dmx_set_level( hb[0], hb[1] );
    }
    if(d > 0) {
       DH2( dmx_get_level( hb[0] ));
       DNL();
    }

  } else if(in[1] == 'c') {
       DU( channel_count, 3 );
       DNL();

  }
}

// F2822ccvv

int dmx_fs20_emu(char *in) {
  uint8_t hb[4], d = 0;

#ifdef DMX_FS20EMU_HC
  if (!strncmp(in+1, DMX_FS20EMU_HC, 4)) {
    memset( hb, 0, sizeof(hb) );
    d = fromhex(in+5, hb, 2);
    if(d == 2) {
      switch (hb[1]) {
        case 0:
          break;
        case 0x1 ... 0xf:
          hb[1] <<= 4;
          break;
        case 0x10:
        case 0x11:
          hb[1] = 0xff;
          break;
        default:
          return 1;
      }
      dmx_set_level( hb[0], hb[1] );
      return 1;
    }
  }
#endif

  return 0;
}

#endif
