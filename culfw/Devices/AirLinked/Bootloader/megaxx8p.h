#ifndef _MEGAxx8_H_
#define _MEGAxx8_H_

#include <avr/wdt.h>
#include <avr/interrupt.h>

/* UART 0 */
#define UART_BAUD_HIGH	UBRR0H
#define UART_BAUD_LOW	UBRR0L
#define UART_STATUS	    UCSR0A
#define UART_TXREADY	UDRE0
#define UART_RXREADY	RXC0
#define UART_DOUBLE	    U2X0
#define UART_CTRL	    UCSR0B
#define UART_CTRL_DATA	((1<<TXEN0) | (1<<RXEN0))
#define UART_CTRL2	    UCSR0C
#define UART_CTRL2_DATA	( (1<<UCSZ01) | (1<<UCSZ00))
#define UART_DATA	    UDR0

#undef WDT_OFF_SPECIAL

static inline void bootloader_wdt_off(void)
{
	cli();
	wdt_reset();
	/* Clear WDRF in MCUSR */
	MCUSR &= ~(1<<WDRF);
	/* Write logical one to WDCE and WDE */
	/* Keep old prescaler setting to prevent unintentional time-out */
	WDTCSR |= (1<<WDCE) | (1<<WDE);
	/* Turn off WDT */
	WDTCSR = 0x00;
}

#endif // #ifndef _MEGA644_H_
