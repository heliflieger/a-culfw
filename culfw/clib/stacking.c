/*
* Support for the SCC stacking transceivers
*
* Written by TOSTi <tosti@busware.de>
* Placed under the modified BSD license
*
*/

#include "board.h"

#ifdef HAS_STACKING

#include "stacking.h"
#include "stringfunc.h"
#include "display.h"
#include "clock.h"
#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>

// Receive a CRLF terminated message from downlink UART1 
// and forward to uplink UART0 by prefixing a '*'

#define BAUD 38400
#define USE_2X
#include <util/setbaud.h>

static char mycmd[256];
static uint8_t mypos = 0;
static volatile uint8_t myEOL = 0;

void stacking_initialize(void) {
  // initialize I/O pins
  DDRD |= _BV( 3 ); // PD3: TX out

  // initialize the USART
  UBRR1H = UBRRH_VALUE;
  UBRR1L = UBRRL_VALUE;
  UCSR1A = 0;
#if USE_2X
  UCSR1A |= _BV(U2X1);
#endif
  UCSR1B = _BV(RXCIE1) | _BV(RXEN1)  | _BV(TXEN1); // RX Complete IRQ enabled, Receiver/Transmitter enable
  UCSR1C = _BV(UCSZ10) | _BV(UCSZ11); // 8 Bit, n, 1 Stop

  mycmd[mypos] = 0;
  myEOL = 0;
}

ISR(USART1_RX_vect) {
  uint8_t errors = UCSR1A;
  uint8_t data   = UDR1;

  if (errors & (_BV(FE1)|_BV(DOR1)))
	return;

  if (myEOL) // unprocessed message?
    return;

  if (data == 13)
    return;

  if (data == 10) {
    // EOL
    mycmd[mypos] = 0;
    myEOL = 1; 
    return;
  }

  mycmd[mypos++] = data;
}

void stacking_task(void) {
  if (!myEOL)
    return;

  // Process message
  DC( '*' );
  DS( mycmd );
  DNL();

  mypos = 0;
  mycmd[mypos] = 0;
  myEOL = 0;
};

void stacking_func(char* in) { 
  // downlink message to UART1 by removing one '*'
  uint8_t i = 1;

  while (in[i]) { 
    while(!(UCSR1A&_BV(UDRE1))); // wait for UART1 clear
    UDR1 = in[i++];
  }

  while(!(UCSR1A&_BV(UDRE1)));
  UDR1 = 13;

  while(!(UCSR1A&_BV(UDRE1)));
  UDR1 = 10;
}

#endif
