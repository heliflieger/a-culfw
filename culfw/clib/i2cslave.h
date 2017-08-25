/*
	i2cslave.c - I2C Slave Communication instead of UART
	
*/

/*
						 (1<<TWEN)|                                 // TWI Interface enabled
             (1<<TWIE)|(1<<TWINT)|                      // Enable TWI Interrupt and clear the flag to send byte
             (1<<TWEA)|(0<<TWSTA)|(0<<TWSTO)|           // TWEA=ACK on Request TWSTO= Slave Release SDA/SCL
             (0<<TWWC)																	//TWWC Write Collision
*/

#define 		TWCR_ACK 			TWCR=(1<<TWEN)|(1<<TWIE)|(1<<TWINT)|(1<<TWEA)|(0<<TWSTA)|(0<<TWSTO)|(0<<TWWC)
#define 		TWCR_NACK 		TWCR=(1<<TWEN)|(1<<TWIE)|(1<<TWINT)|(0<<TWEA)|(0<<TWSTA)|(0<<TWSTO)|(0<<TWWC)
#define 		TWCR_RELEASE 	TWCR=(1<<TWEN)|(1<<TWIE)|(1<<TWINT)|(1<<TWEA)|(0<<TWSTA)|(1<<TWSTO)|(0<<TWWC)

void I2CSlave_init(void);
void I2CSlave_task(void);