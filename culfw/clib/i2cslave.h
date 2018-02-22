/*
	i2cslave.h - I2C Slave Communication instead of UART
	
	19.01.2017	-	Diggen85 (Benny_Stark@live.de)
							-	Add Communication as I2C Slave
	19.02.2018	- Defined a Macro for TWCR_INIT, TWCR_OFF to reuse in rf_send.c
							-	Fixed sending from rf_send.c - Disable/Enable Interrupt
								Thanks to Markus
*/

/*
  TWCR =(1<<TWEN)|         		//TWI enabled.
		(1<<TWIE)|(0<<TWINT)|   //Enable TWI Interrupt and dont touch TWINT,else it wont work
		(1<<TWEA)|				//Prepare to ACK next time the Slave is addressed.
		(0<<TWSTA)|(0<<TWSTO)|  //No Start as Slave / Stop as Slave would release   
		(0<<TWWC); 
*/
#define	TWCR_INIT		TWCR=(1<<TWEN)|(1<<TWIE)|(0<<TWINT)|(1<<TWEA)|(0<<TWSTA)|(0<<TWSTO)|(0<<TWWC) 
#define	TWCR_OFF		TWCR=(0<<TWEN)|(0<<TWIE)|(0<<TWINT)|(0<<TWEA)|(0<<TWSTA)|(0<<TWSTO)|(0<<TWWC)

#define	TWCR_ACK		TWCR=(1<<TWEN)|(1<<TWIE)|(1<<TWINT)|(1<<TWEA)|(0<<TWSTA)|(0<<TWSTO)|(0<<TWWC)
#define TWCR_NACK 		TWCR=(1<<TWEN)|(1<<TWIE)|(1<<TWINT)|(0<<TWEA)|(0<<TWSTA)|(0<<TWSTO)|(0<<TWWC)
#define TWCR_RELEASE 	TWCR=(1<<TWEN)|(1<<TWIE)|(1<<TWINT)|(1<<TWEA)|(0<<TWSTA)|(1<<TWSTO)|(0<<TWWC)

void I2CSlave_init(void);
void I2CSlave_task(void);