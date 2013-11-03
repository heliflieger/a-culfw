/*  
* Support for the KWL Helios RS485 emulation
*
* Currently assumes that the RS485 port is connected to the second
* UART (as is the case with the CUNO2)
*  
* Written by TOSTi <tosti@busware.de>
* Placed under the modified BSD license  
*  
*/   

#include "board.h"   

#ifdef HAS_HELIOS

#ifdef HAS_DMX 
#error "You can't have HAS_DMX when HAS_HELIOS is enabled"
#endif
       
#ifdef HAS_HM485 
#error "You can't have HAS_HM485 when HAS_HELIOS is enabled"
#endif
       
#include "helios.h"   
#include "stringfunc.h"   
#include "display.h"   
#include <string.h>
#include <avr/io.h>   
#include <avr/interrupt.h>   
       
// PD2 (RXD1) and PD3 (TXD1) are connected to the RS485 transceiver's 
// RXD and TXD; PA3 is connected to REN/TXEN (high = transmit)   

static uint8_t helios_crc;

#define BAUD 9600
#include <util/setbaud.h>

void helios_initialize(void) {   
      // initialize I/O pins   
      DDRA |= _BV( 3 ); // PA3: TXEN of the RS485 transceiver
      PORTA &= ~_BV(3);
      DDRD |= _BV( 3 ); // PD3: TX out
       
      // initialize the USART
      UBRR1H = UBRRH_VALUE;
      UBRR1L = UBRRL_VALUE;
      UCSR1A = 0;   
      #if USE_2X
      UCSR1A |= _BV(U2X1);
      #endif      
      UCSR1B = _BV(RXCIE1) | _BV(RXEN1); // RX Complete IRQ enabled, Receiver Enable 
      UCSR1C = _BV(UCSZ10) | _BV(UCSZ11); // 8 Bit, n, 1 Stop  
}   

static void usart_send(uint8_t b) {
        while(!(UCSR1A&_BV(UDRE1)));
        UDR1=b;
	helios_crc += b;
}

static void helios_set(uint8_t reg, uint8_t val) {

        // Enable UART TX mode
        UCSR1B&=~_BV(RXEN1);
        UCSR1B|=_BV(TXEN1);

        PORTA |= _BV(3);

	helios_crc = 0;
	usart_send(0x01); 
	usart_send(0x2f); 
	usart_send(0x20); 
	usart_send(reg); 
	usart_send(val); 
	usart_send(helios_crc); 

	helios_crc = 0;
	usart_send(0x01); 
	usart_send(0x2f); 
	usart_send(0x10); 
	usart_send(reg); 
	usart_send(val); 
	usart_send(helios_crc); 

	helios_crc = 0;
	usart_send(0x01); 
	usart_send(0x2f); 
	usart_send(0x11); 
	usart_send(reg); 
	usart_send(val); 
	uint8_t c = helios_crc;
	usart_send(c); 
	usart_send(c); 

        PORTA &= ~_BV(3);
        // Enable UART RX mode
        UCSR1B&=~_BV(TXEN1);
        UCSR1B|=_BV(RXEN1);

}

ISR(USART1_RX_vect) {
	uint8_t errors=UCSR1A;
	uint8_t data=UDR1;
	
	if(errors&(_BV(FE1)|_BV(DOR1)|_BV(UPE1))) {
		// Terminate current message, if any
		return;
	}

//        DH2(data); DC(' ');
	
	switch(data) {
	}
}   

void helios_task(void) {
}

void helios_func(char *in) {
	switch(in[1]) {
		default:
			DS_P(PSTR("nothing yet"));
			DNL();
	}
}

int helios_fs20_emu(char *in) {
  uint8_t hb[4], d = 0;

#ifdef HELIOS_EMU_HC
  if (!strncmp(in+1, HELIOS_EMU_HC, 4)) {
    memset( hb, 0, sizeof(hb) );
    d = fromhex(in+5, hb, 2);
    if(d == 2) {
      switch (hb[1]) {
        case 0:
	  helios_set( 0x29, 0 );
          break;
        case 0x1:
        case 0x2:
	  helios_set( 0x29, 1 );
          break;
        case 0x3:
        case 0x4:
	  helios_set( 0x29, 3 );
          break;
        case 0x5:
        case 0x6:
	  helios_set( 0x29, 7 );
          break;
        case 0x7:
        case 0x8:
	  helios_set( 0x29, 0xf );
          break;
        case 0x9:
        case 0xa:
	  helios_set( 0x29, 0x1f );
          break;
        case 0xb:
        case 0xc:
	  helios_set( 0x29, 0x3f );
          break;
        case 0xd:
        case 0xe:
	  helios_set( 0x29, 0x7f );
          break;
        case 0xf:
        case 0x10:
        case 0x11:
	  helios_set( 0x29, 0xff );
          break;
        default:
          return 1;
      }
      return 1;
    }
  }
#endif

  return 0;
}

#endif

