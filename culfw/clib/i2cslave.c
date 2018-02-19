/*
	i2cslave.c - I2C Slave Communication instead of UART
	
	19.01.2017	-	Diggen85 (Benny_Stark@live.de)
							-	Add Communication as I2C Slave
	19.02.2018	- Defined a Macro for TWCR_INIT, TWCR_OFF to reuse in rf_send.c
							-	Fixed sending from rf_send.c - Disable/Enable Interrupt
								Thanks to Markus
*/
#include <util/twi.h>
#include <avr/interrupt.h>

#include "i2cslave.h"
#include "display.h"
#include "ringbuffer.h"
#include "ttydata.h"

#include "board.h"


void I2CSlave_init(void)
{
#if defined(I2CSLAVE_HWADDR)
	//Set Address Pins as Input and Pullups on
	I2CSLAVE_ADDR_DDR 	&= ~((1<<I2CSLAVE_ADDR_A1)|(1<<I2CSLAVE_ADDR_A0));
	I2CSLAVE_ADDR_PORT	|= 	((1<<I2CSLAVE_ADDR_A1)|(1<<I2CSLAVE_ADDR_A0));
	
	//Reading Address Bits and add them to i2cSlaveAddr ( I2CSLAVE_ADDR_BASE )
	if (I2CSLAVE_ADDR_PIN & (1<<I2CSLAVE_ADDR_A0)) 
		i2cSlaveAddr |= (1<<0);
	else 
		i2cSlaveAddr &= ~(1<<0);
	if (I2CSLAVE_ADDR_PIN & (1<<I2CSLAVE_ADDR_A1)) 
		i2cSlaveAddr |= (1<<1);
	else 
		i2cSlaveAddr &= ~(1<<1);
	
	// Disable Pullups after getting Address - less stress to Pullups
	I2CSLAVE_ADDR_PORT	&= 	~((1<<I2CSLAVE_ADDR_A1)|(1<<I2CSLAVE_ADDR_A0));
#endif

	TWAR =  (i2cSlaveAddr<<1);		//Set Slave Address / Shift left because BIT 1 = General CALL
	TWCR_INIT;
}

void I2CSlave_task(void) 
{
     input_handle_func(DISPLAY_USB);
}

ISR(TWI_vect)
{
	//Simple State Maschine
	switch (TW_STATUS)
	{
		//Send Data from tty_buffer
		
		//SLA+R - ACK has returned - send Data
		case TW_ST_SLA_ACK: //A8
		 //ACK Returned for Data - send Data
		case TW_ST_DATA_ACK: //B8
			if (TTY_Tx_Buffer.nbytes)
				TWDR = rb_get(&TTY_Tx_Buffer);
			else
				TWDR = 0x04; //ASCII EOT
			TWCR_ACK;
			break;
		
	
		//Receive Data and write to tty_buffer
		
		//SLA+W - Return ACK
		case TW_SR_SLA_ACK: //60
			TWCR_ACK;
			break;
		
		//ACK Returned for Data - Get Data and write to tty_buffer
		case TW_SR_DATA_ACK: //80
			rb_put(&TTY_Rx_Buffer, TWDR);
			TWCR_ACK;
			break;
		
		//All Other States Release
		case TW_SR_STOP: //A0	
		case TW_ST_DATA_NACK: //C0 NACK Returned - no more Data
		case TW_ST_LAST_DATA: //C8 Lasz Data Transmitted, ACK
		case TW_SR_DATA_NACK: //88*/
		default:
			//Release
			TWCR_RELEASE;
			break;
	}
}
