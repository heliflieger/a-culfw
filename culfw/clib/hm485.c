/*  
* Support for the RS485 based "HM485" protocol (also called Homematic Wired)
*
* Currently assumes that the RS485 port is connected to the second
* UART (as is the case with the CUNO2)
*  
* Written by Oliver Wagner <owagner@vapor.com>
* Placed under the modified BSD license  
*  
*/   

#include "board.h"   

#ifdef HAS_HM485
       
#include "hm485.h"   
#include "stringfunc.h"   
#include "display.h"   
#include <string.h>
#include <avr/io.h>   
#include <avr/interrupt.h>   
       
// PD2 (RXD1) and PD3 (TXD1) are connected to 75176's RXD and TXD; PA3 is   
// connected to REN/TXEN (high = transmit)   

#define BAUD 19200
#include <util/setbaud.h>

#define HM485_MSG_SOM 0xfd
#define HM485_MSG_ESC 0xfc

#define HM485_BUFFERLEN 80

void hm485_initialize(void)   
{   
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
      UCSR1C = _BV(UPM11) | _BV(UCSZ10) | _BV(UCSZ11); // 8 Bit, Even Parity (!), 1 Stop  
}   

static uint8_t rindex, rstate;
static uint8_t rmsg[HM485_BUFFERLEN], rbuf[HM485_BUFFERLEN];

#define RSTATE_ESC 1
#define RSTATE_OK 2
#define RSTATE_ERROR 4
#define RSTATE_OVERFLOW 8
#define RSTATE_ACTIVE 128

ISR(USART1_RX_vect)   
{
	uint8_t data=UDR1;
	
	switch(data)
	{
		case HM485_MSG_SOM:
			// A new message starts
			rindex=0;
			rstate=0;
			break;
			
		case HM485_MSG_ESC:
			// Next byte will be escaped
			rstate|=RSTATE_ESC;
			break;
			
		default:
			if(rindex==HM485_BUFFERLEN)
			{
				rstate|=RSTATE_OVERFLOW;
				break;
			}
			if(rstate&RSTATE_ESC)
			{
				data|=0x80;
				rstate&=~RSTATE_ESC;
			}
			rbuf[rindex++]=data;
			if(rindex>9)
			{
				// Do we have a valid message?
				if(rbuf[9]+10==rindex)
				{
					// We do
					if(rstate&RSTATE_OK)
					{
						// Old message not processed yet...
						rstate=RSTATE_OVERFLOW;
					}
					else
					{
						memcpy(rmsg,rbuf,rindex);
						rstate=RSTATE_OK;
					}
				}
			}
			break;
	}
}   

void hm485_task(void)
{
	if(rstate&RSTATE_OK)
	{
		// We have a complete message
		DC('H');
		// First dump the two addresses and the control byte
		for(uint8_t c=0;c<9;c++)
		{
			DH2(rmsg[c]);
		}
		// Then dump the payload, if any, without CRC
		for(uint8_t c=10;c<=rmsg[9]+9-2;c++)
		{
			DH2(rmsg[c]);	
		}
		DNL();
		rstate&=~RSTATE_OK;
	}
}

void hm485_func(char *in)
{
	// Todo
}

#endif
