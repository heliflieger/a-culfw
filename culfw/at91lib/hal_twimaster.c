#include <board.h>

#include "i2cmaster.h"
#include "delay.h"
#include "hal_gpio.h"

#ifdef HAS_ONEWIRE
#define TWI_DELAY           my_delay_us(4)

#define TWI_SCL_PIN       CCtransceiver[i2cPort].pin[CC_Pin_In]
#define TWI_SCL_BASE      CCtransceiver[i2cPort].base[CC_Pin_In]
#define TWI_SDA_PIN       CCtransceiver[i2cPort].pin[CC_Pin_CS]
#define TWI_SDA_BASE      CCtransceiver[i2cPort].base[CC_Pin_CS]

#define TWI_SET_SCL         TWI_SCL_BASE->PIO_SODR =  _BV(TWI_SCL_PIN)
#define TWI_CLEAR_SCL       TWI_SCL_BASE->PIO_CODR =  _BV(TWI_SCL_PIN)

#define TWI_SET_SDA         TWI_SDA_BASE->PIO_SODR =  _BV(TWI_SDA_PIN)
#define TWI_CLEAR_SDA       TWI_SDA_BASE->PIO_CODR =  _BV(TWI_SDA_PIN)

/*************************************************************************
 Initialization of the I2C bus interface. Need to be called only once
*************************************************************************/
void i2c_init(void)
{

  //TWI_SCL_PIN
  TWI_SCL_BASE->PIO_PPUER = _BV(TWI_SCL_PIN); //Enable pullup
  TWI_SCL_BASE->PIO_OER =   _BV(TWI_SCL_PIN); //Enable output
  TWI_SCL_BASE->PIO_PER =   _BV(TWI_SCL_PIN); //Enable PIO control

  //TWI_SDA_PIN
  TWI_SDA_BASE->PIO_PPUER = _BV(TWI_SDA_PIN); //Enable pullup
  TWI_SDA_BASE->PIO_OER =   _BV(TWI_SDA_PIN); //Enable output
  TWI_SDA_BASE->PIO_MDER =  _BV(TWI_SDA_PIN); //Enable multi drive
  TWI_SDA_BASE->PIO_PER =   _BV(TWI_SDA_PIN); //Enable PIO control

  TWI_SET_SCL;
  TWI_SET_SDA;


}/* i2c_init */


/*************************************************************************	
  Issues a start condition and sends address and transfer direction.
  return 0 = device accessible, 1= failed to access device
*************************************************************************/
unsigned char i2c_start(unsigned char address)
{
  // send START condition
  TWI_CLEAR_SDA;
  TWI_DELAY;
  TWI_CLEAR_SCL;
  TWI_DELAY;

  // send device address
  return i2c_write(address);

}/* i2c_start */


/*************************************************************************
 Issues a start condition and sends address and transfer direction.
 If device is busy, use ack polling to wait until device is ready
 
 Input:   address and transfer direction of I2C device
*************************************************************************/
void i2c_start_wait(unsigned char address)
{

}/* i2c_start_wait */


/*************************************************************************
 Issues a repeated start condition and sends address and transfer direction 

 Input:   address and transfer direction of I2C device
 
 Return:  0 device accessible
          1 failed to access device
*************************************************************************/
unsigned char i2c_rep_start(unsigned char address)
{
    return i2c_start( address );

}/* i2c_rep_start */


/*************************************************************************
 Terminates the data transfer and releases the I2C bus
*************************************************************************/
void i2c_stop(void)
{
  // send stop condition
  TWI_DELAY;
  TWI_SET_SCL;
  TWI_DELAY;
  TWI_SET_SDA;

}/* i2c_stop */


/*************************************************************************
  Send one byte to I2C device
  
  Input:    byte to be transfered
  Return:   0 write successful 
            1 write failed
*************************************************************************/
unsigned char i2c_write( unsigned char data )
{	
  uint32_t   ack=0;

  // send data
  for(uint8_t x=0; x<8;x++) {
    if(data & 0x80)
      TWI_SET_SDA;
    else
      TWI_CLEAR_SDA;

    TWI_DELAY;
    TWI_SET_SCL;
    TWI_DELAY;
    TWI_CLEAR_SCL;

    data <<= 1;
  }

  // get ack
  TWI_SET_SDA;
  TWI_DELAY;
  TWI_SET_SCL;
  TWI_DELAY;
  ack = ((TWI_SDA_BASE)->PIO_PDSR) & _BV(TWI_SDA_PIN);
  TWI_CLEAR_SCL;
  TWI_CLEAR_SDA;

  if (ack)
    return 1;

  return 0;
}/* i2c_write */

/*************************************************************************
  Read one byte from the I2C device

  Input:   0 send no Ack
           1 send Ack
  Return:  byte read from I2C device
*************************************************************************/
unsigned char i2c_Read( unsigned char ack )
{
  uint8_t   data=0;

  // receive
  TWI_SET_SDA;
  for(uint8_t x=0; x<8;x++) {
    data <<= 1;
    TWI_DELAY;
    TWI_SET_SCL;
    TWI_DELAY;
    if (((TWI_SDA_BASE)->PIO_PDSR) & _BV(TWI_SDA_PIN)) {
      data |= 0x01;
    }
    TWI_CLEAR_SCL;


  }

  // send ack
  if(ack)
    TWI_CLEAR_SDA;
  else
    TWI_SET_SDA;
  TWI_DELAY;
  TWI_SET_SCL;
  TWI_DELAY;
  TWI_CLEAR_SCL;
  TWI_CLEAR_SDA;

  return data;
}/* i2c_write */

/*************************************************************************
 Read one byte from the I2C device, request more data from device 
 
 Return:  byte read from I2C device
*************************************************************************/
unsigned char i2c_readAck(void)
{
  return i2c_Read(1);
}/* i2c_readAck */


/*************************************************************************
 Read one byte from the I2C device, read is followed by a stop condition 
 
 Return:  byte read from I2C device
*************************************************************************/
unsigned char i2c_readNak(void)
{
  return i2c_Read(0);
}/* i2c_readNak */

#endif
