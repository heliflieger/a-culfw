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
       
// PD2 (RXD1) and PD3 (TXD1) are connected to the RS485 transceiver's 
// RXD and TXD; PA3 is connected to REN/TXEN (high = transmit)   

#define BAUD 19200
#include <util/setbaud.h>

#define HM485_MSG_SOM 0xfd
#define HM485_MSG_ESC 0xfc

#define HM485_BUFFERLEN 75
#define HM485_LOG_ERRORS
#ifdef HM485_LOG_ERRORS
#define LOGERROR(n) n++;
#else
#define LOGERROR(n)
#endif

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

static uint32_t crc;
static void crc_init(void)
{
	crc=0xffff00;	
}
static void crc_update(uint8_t v)
{
	crc|=v;
	for(uint8_t b=0;b<8;b++)
	{
		crc<<=1;
		if(crc&0x1000000)
			crc^=0x100200;
		crc&=0xffffff;		
	}
}


static uint8_t rindex;
static volatile uint8_t rstate;
static uint8_t rmsg[HM485_BUFFERLEN], rbuf[HM485_BUFFERLEN];
#ifdef HM485_LOG_ERRORS
static uint16_t error_frame, error_parity, error_overrun, error_data, error_crc, error_datasize;
static uint16_t msg_cnt, msg_sent;
#endif

#define RSTATE_ESC 1
#define RSTATE_MSG 2
#define RSTATE_COMPLETE 128

ISR(USART1_RX_vect)   
{
	uint8_t errors=UCSR1A;
	uint8_t data=UDR1;
	
	if(errors&(_BV(FE1)|_BV(DOR1)|_BV(UPE1)))
	{
		// Terminate current message, if any
		rstate&=~RSTATE_MSG;
#ifdef HM485_LOG_ERRORS
		if(errors&_BV(FE1))
			error_frame++;
		if(errors&_BV(DOR1))
			error_overrun++;
		if(errors&_BV(UPE1))
			error_parity++;
#endif
		return;
	}
	
	switch(data)
	{
		case HM485_MSG_SOM:
			// A new message starts
#ifdef HM485_LOG_ERRORS			
			if(rstate&RSTATE_MSG)
			{
				// There was an incomplete message
				error_data++;
			}
#endif			
			rindex=0;
			rstate|=RSTATE_MSG;
			break;
			
		case HM485_MSG_ESC:
			if(!(rstate&RSTATE_MSG))
			{
				// We're not actually expecting data
				LOGERROR(error_data);
				break;
			}
			// Next byte will be escaped
			rstate|=RSTATE_ESC;
			break;
			
		default:
			if(!(rstate&RSTATE_MSG))
			{
				// We're not actually expecting data
				LOGERROR(error_data);
				break;
			}
			if(rindex==HM485_BUFFERLEN)
			{
				// Message too large
				rstate&=~RSTATE_MSG;
				LOGERROR(error_datasize);
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
				// Do we have a complete message?
				if(rbuf[9]+10==rindex)
				{
					// We do
					if(rstate&RSTATE_COMPLETE)
					{
						// Old message not processed yet...
						LOGERROR(error_overrun);
					}
					else
					{
						memcpy(rmsg,rbuf,rindex);
						rstate|=RSTATE_COMPLETE;
						LOGERROR(msg_cnt);
					}
					rstate&=~RSTATE_MSG;
				}
			}
			break;
	}
}   

void hm485_task(void)
{
	if(rstate&RSTATE_COMPLETE)
	{
		// Check CRC
		crc_init();
		crc_update(HM485_MSG_SOM);
		for(int c=0;c<=rmsg[9]+9;c++)
			crc_update(rmsg[c]);
		if(crc!=0)
		{
			// CRC check failed
			LOGERROR(error_crc);
			rstate&=~RSTATE_COMPLETE;
			return;
		}
		
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
		rstate&=~RSTATE_COMPLETE;
	}
}

static void displaystatus(void)
{
	DS_P(PSTR("SENT:"));
	DU(msg_sent,0);
	DS_P(PSTR(" RECV:"));
	DU(msg_cnt,0);
	DS_P(PSTR(" FE:"));
	DU(error_frame,0);
	DS_P(PSTR(" PE:"));
	DU(error_parity,0);
	DS_P(PSTR(" OV:"));
	DU(error_overrun,0);
	DS_P(PSTR(" CRC:"));
	DU(error_crc,0);
	DS_P(PSTR(" DATA:"));
	DU(error_data,0);
	DS_P(PSTR(" SIZE:"));
	DU(error_datasize,0);
	DNL();
}

static void usart_beginsend(void)
{
	// If we're currently receiving data, wait
	// If the receiver got somehow stuck, this will eventually
	// trigger a WDT reset
	for(;;)
	{
		if(!(rstate&RSTATE_MSG))
			break;
	}
	// Enable UART TX mode
	UCSR1B&=~_BV(RXEN1);
	UCSR1B|=_BV(TXEN1);
	// Enable TX mode for the RS485 transceiver
	PORTA |= _BV(3);
}

static void usart_endsend(void)
{
	// Wait for the last byte to be sent out
	UCSR1A&=~TXC1;
	while(!(UCSR1A&_BV(UDRE1)));
	while(!(UCSR1A&_BV(TXC1)));
	// Disable RS485 transceiver
	PORTA &= ~_BV(3);
	// Enable UART RTX mode
	UCSR1B&=~_BV(TXEN1);
	UCSR1B|=_BV(RXEN1);
}

static void usart_send(uint8_t b)
{
	while(!(UCSR1A&_BV(UDRE1)));
	UDR1=b;
}

static void usart_sendescaped(uint8_t b)
{
	if(b==0xfc || b==0xfd || b==0xfe)
	{
		usart_send(0xfc);
		usart_send(b&0x7f);
	}
	else
		usart_send(b);
}


static void usart_sup(uint8_t b)
{
	crc_update(b);
	usart_sendescaped(b);	
}

void sendmsg(char *m)
{
	uint8_t msg[HM485_BUFFERLEN];
	
	int l=fromhex(m,msg,sizeof(msg));
	if(l<9)
	{
		DS_P(PSTR("too short, minimum 9 bytes: ddddddccssssssss"));
		DNL();
		return;
	}
	usart_beginsend();
	
	// Needs to be sent unescaped
	usart_send(HM485_MSG_SOM);
	crc_init();
	crc_update(HM485_MSG_SOM);
	
	// Now send the standard message data
	for(int c=0;c<9;c++)
		usart_sup(msg[c]);			
	// Payload len
	usart_sup(l-9+2);
	// and Payload
	for(int c=9;c<l;c++)
		usart_sup(msg[c]);
	crc_update(0);
	crc_update(0);
	usart_sendescaped(crc>>16);
	usart_sendescaped(crc>>8);

	LOGERROR(msg_sent);
	
	usart_endsend();	
}

void hm485_func(char *in)
{
	switch(in[1])
	{
#ifdef HM485_LOG_ERRORS		
		case '?':
			displaystatus();
			break;
#endif
		case 's':
			sendmsg(&in[2]);
			break;
			
		default:
			DS_P(PSTR("H? status, Hsaabbcc... send message aabbcc..."));
			DNL();
	}
}

#endif
