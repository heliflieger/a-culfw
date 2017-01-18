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
       
#include <avr/interrupt.h>
#include <avr/io.h>
#include <string.h>

#include "clock.h"
#include "display.h"
#include "helios.h"
#include "stringfunc.h"
       
// PD2 (RXD1) and PD3 (TXD1) are connected to the RS485 transceiver's 
// RXD and TXD; PA3 is connected to REN/TXEN (high = transmit)   

static uint8_t helios_crc, msgpos, helios_power, helios_debug;
static uint8_t msg[10], helios_temp[4];
static uint32_t helios_next;

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

      msgpos = 0;
      helios_next = -1;
      helios_debug = 0;

      memset( helios_temp, 0, sizeof(helios_temp));
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

	// check SOM
	if ((msgpos == 0) && (data != 1)) 
		return;

	// check EOM & CRC
	if (msgpos == 5) {

		for (uint8_t i=0;i<5;i++)
			data -= msg[i];

		if (data) {
			// CRC error
			if (helios_debug)
				DC( '*' );
		} else {
			if (helios_debug)
			for (uint8_t i=0;i<5;i++) {
				DH2( msg[i] );
				DC( ' ' );
			}
			// check value updates
			if (msg[1] == 0x11) {
				switch(msg[3]) {
					case 0x29: // Drehzahl
					  
						if (helios_power != msg[4])
							helios_next = 0; // force FHEM update

						helios_power = msg[4];
						break;

				        case 0x32:
				        case 0x33:
				        case 0x34:
				        case 0x35:
						if (helios_temp[msg[3]-0x32] != msg[4])
						        helios_next = 0; // force FHEM update
					  
					        helios_temp[msg[3]-0x32] = msg[4];
					        break;
				}
			}
		}

		if (helios_debug)
			DNL();

		msgpos = 0;
		return;
        }

        msg[msgpos++] = data;

}   

void helios_task(void) {
        char   sign = 0;
	int    temp = 0;
	double tempf = 0.0;
  
	if (helios_next>ticks) 
		return;

	DC( 'F' );
	DS_P(PSTR(HELIOS_EMU_HC));
	DH2( 0 );

	switch (helios_power) {
		case 0x1:
			DH2( 0x1 );
			break;
		case 0x3:
			DH2( 0x3 );
			break;
		case 0x7:
			DH2( 0x5 );
			break;
		case 0xf:
			DH2( 0x7 );
			break;
		case 0x1f:
			DH2( 0x9 );
			break;
		case 0x3f:
			DH2( 0xb );
			break;
		case 0x7f:
			DH2( 0xd );
			break;
		case 0xff:
			DH2( 0xf );
			break;
	}

	DH2( 0xff ); // RSSI
	DNL();

	for (uint8_t i=0;i<4;i++) {
	  
	  if (helios_temp[i]) {
	    tempf = (helios_temp[i]-100)/0.3;
	    temp  = (int) tempf;
	    sign  = (temp<0) ? '8' : '0';
	    
	    DC('H');
	    DH2(0x33);          //this needs to be coded more flexable
	    DH2(0x10+i);
	    DC(sign);		//Sign-Bit (needs to be 8 for negative
	    DC('1'); 		//HMS Type (only Temp)
	    //Temp under 10 degs
	    sign  =  (char) ( ( (uint8_t) ((temp / 10 ) % 10) )&0x0F ) + '0';
	    DC(sign);
	    //Degrees below 1  
	    sign  =  (char) ( ( (uint8_t)  (temp % 10 ) 	  )&0x0F ) + '0';
	    DC(sign);
	    DC('0');
	    //Temp over 9 degs
	    sign  =  (char) ( ( (uint8_t) ((temp / 100) % 10) )&0x0F ) + '0';
	    DC(sign);
	    DC('0');DC('0');DC('F');DC('F');                                            //Humidity & RSSI
	    DNL();
	  }
	}
	
	helios_next = ticks + 41000; // roughly 5 min
}

void helios_func(char *in) {
	switch(in[1]) {
		case 'D':
			helios_debug = 1;
			break;
		case 'd':
			helios_debug = 0;
			break;
		default:
			DS_P(PSTR("use d/D to toggle raw message dump"));
			DNL();
	}
}

int helios_fs20_emu(char *in) {
#ifdef HELIOS_EMU_HC
  uint8_t hb[4], d = 0;
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

