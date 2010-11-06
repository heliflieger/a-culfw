#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/boot.h>
#include "board.h"

#include "fncollection.h"
#include "stringfunc.h"
#include "timer.h"
#include "display.h"
#include "delay.h"

#include "onewire.h"
#include "i2cmaster.h"


unsigned char
onewire_BusReset(void)
{
	unsigned char status;
	// send 1-Wire bus reset command
	ds2482SendCmd(DS2482_CMD_1WRS);
	// wait for bus reset to finish, and get status
	status = onewire_BusyWait();
	// return state of the presence bit
	if (status & DS2482_STATUS_PPD) {
		DC('D'); DC(':'); DC(' '); DC('1');
 	  DNL();
	} else {
		DC('D'); DC(':'); DC(' '); DC('0');
 	  DNL();		
	}
	return (status & DS2482_STATUS_PPD);
}
 
unsigned char 
onewire_BusyWait(void)
{
	unsigned char status;
	// set read pointer to status register
	ds2482SendCmdArg(DS2482_CMD_SRP, DS2482_READPTR_SR);
	// check status until busy bit is cleared
	do
	{
		i2cMasterReceive(DS2482_I2C_ADDR, 1, &status);
	} while(status & DS2482_STATUS_1WB);
	// return the status register value
	return status;
}

void 
onewire_BusWriteByte(unsigned char data)
{
	// wait for DS2482 to be ready
	onewire_BusyWait();
	// send 1WWB command
	ds2482SendCmdArg(DS2482_CMD_1WWB, data);
}

unsigned char
onewire_BusReadByte(void)
{
	unsigned char data;
	// wait for DS2482 to be ready
	onewire_BusyWait();
	// send 1WRB command
	ds2482SendCmd(DS2482_CMD_1WRB);
	// wait for read to finish
	onewire_BusyWait();
	// set read pointer to data register
	ds2482SendCmdArg(DS2482_CMD_SRP, DS2482_READPTR_RDR);
	// read data
	i2cMasterReceive(DS2482_I2C_ADDR, 1, &data);
	// return data
	return data;
}


void 
onewire_ReadROMCode(void)
{
  int n;
  char dat[9];

  //ow_reset();
  onewire_BusWriteByte(0x33);
  for (n=0;n<8;n++){dat[n]=onewire_BusReadByte();}
  DC('R'); DC(':'); DU(dat[7],4);DU(dat[6],4);DU(dat[5],4);DU(dat[4],4);DU(dat[3],4);DU(dat[2],4);DU(dat[1],4);DU(dat[0],4);
  DNL();
}

void 
onewire_ParasitePowerOn(void)
{
  onewire_BusReset();
  onewire_BusWriteByte(0xCC); //Skip ROM
  onewire_BusWriteByte(0x44); // Start Conversion
}

void 
onewire_ReadTemperature(void)
{
  char get[10];
  int k;
  int temp;
  
  onewire_BusReset();
  
  onewire_BusWriteByte(0xCC); // Skip ROM
  onewire_BusWriteByte(0xBE); // Read Scratch Pad
  for (k=0;k<9;k++){get[k]=onewire_BusReadByte();}
  //printf("\n ScratchPAD DATA = %X%X%X%X%X\n",get[8],get[7],get[6],get[5],get[4],get[3],get[2],get[1],get[0]);

	temp = (get[1] *256 + get[0]) / 16;
/*  
  temp = (get[1] << 8) + get[0];
  SignBit = get[1] & 0x80;  // test most sig bit 
  if (SignBit) // negative
  {
    temp = (temp ^ 0xffff) + 1; // 2's comp
  }

  */
  	
	DC('T'); DC('e'); DC('m'); DC('p'); DC(':'); DC(' '); DU(get[0], 4); DU(get[1],4); DC(':'); DU(temp, 4); DC('C');
	DNL();
  //printf( "\nTempC= %d degrees C\n", (int)temp_lsb ); // print temp. C
}



void
onewire_func(char *in)
{
  if(in[1] == 'i') {
    ds2482Init();

  } else if(in[1] == 'r') {
		  if (in[2] == 'm') 
		   	{ ds2482Reset();
      	} 
      else if(in[2] == 'b') 
      	{ onewire_BusReset();
        }
  } else if(in[1] == 'c') {
  			onewire_ReadROMCode();   	
  } else if(in[1] == 't') {
  			onewire_ReadTemperature();   	
  } else if(in[1] == 'p') {
  			onewire_ParasitePowerOn();   	
  } /*else if(in[1] == 'd') {
    eth_debug = (eth_debug+1) & 0x3;
    DH2(eth_debug);
    DNL();

  } else if(in[1] == 'n') {
    ntp_sendpacket();

  }*/
}



// DS2482 Command functions
// These functions are used to address the DS2482 directly (via I2C)
// The idea is to send Commands and Commands with arguments

void
ds2482Init(void)
{
	unsigned char ret;
  //we know, that I2C is already initialized and running
   ret = i2c_start(DS2482_I2C_ADDR+I2C_WRITE);       // set device address and write mode  
   if ( ret ) {
      /* failed to issue start condition, possibly no device found */
      i2c_stop(); //release bus, as this has failed
      DC('F'); DC('a'); DC('i'); DC('l'); DC('e'); DC('d'); DC(' '); DC('I'); DC('2'); DC('C');
      DNL();
   } else {
   	i2c_stop(); //release bus, as this was just an initialization
   	DC('O'); DC('K');
   	DNL();
  }
}

void
ds2482Reset(void)
{
	unsigned char ret;
	ret = ds2482SendCmd(DS2482_CMD_DRST);
	if (ret == I2C_OK) {
		 DC('O'); DC('K');
 	   DNL();
	}
}

unsigned char 
ds2482SendCmd(unsigned char cmd)
{
	unsigned char data;
	unsigned char i2cStat;

	// send command
	i2cStat = i2cMasterSend(DS2482_I2C_ADDR, 1, &cmd);
	if(i2cStat == I2C_ERROR_NODEV)
	{
    DC('I'); DC('2'); DC('C'); DC(' '); DC('F'); DC('a'); DC('i'); DC('l'); DC('e'); DC('d');
    DNL();
		return i2cStat;
	}
	// check status
	i2cMasterReceive(DS2482_I2C_ADDR, 1, &data);
  //	rprintf("Cmd=0x%x  Status=0x%x\r\n", cmd, data);
	return (I2C_OK);
}

unsigned char 
ds2482SendCmdArg(unsigned char cmd, unsigned char arg)
{
	unsigned char data[2];
	unsigned char i2cStat;

	// prepare command
	data[0] = cmd;
	data[1] = arg;
	// send command
	i2cStat = i2cMasterSend(DS2482_I2C_ADDR, 2, data);
	if(i2cStat == I2C_ERROR_NODEV)
	{
    DC('I'); DC('2'); DC('C'); DC(' '); DC('F'); DC('a'); DC('i'); DC('l'); DC('e'); DC('d');
    DNL();
		return i2cStat;
	}
	// check status
	i2cMasterReceive(DS2482_I2C_ADDR, 1, data);
	//	rprintf("Cmd=0x%x  Arg=0x%x  Status=0x%x\r\n", cmd, arg, data[0]);

	return (I2C_OK);
}


// I2C Send & Receive functions 
// send and receives data of a well defined length via the I2C bus to a specified device
// Handling of specific devices on the I2C bus is done above here (DS2482)

unsigned char 
i2cMasterSend(unsigned char deviceAddr, unsigned char length, unsigned char* data)
{
	 unsigned char ret;
	 
   ret = i2c_start(deviceAddr+I2C_WRITE);       // set device address and write mode  
   if ( ret ) {
      /* failed to issue start condition, possibly no device found */
      i2c_stop(); //release bus, as this has failed
      DC('F'); DC('a'); DC('i'); DC('l'); DC('e'); DC('d'); DC(' '); DC('I'); DC('2'); DC('C');
      DNL();
      return ret;
   } else {   
      // send data
      while(length)
      {
          i2c_write( *data++ );  // write data to DS2482
           length--;
      }
      i2c_stop();  
      return I2C_OK;
		}
}

unsigned char
i2cMasterReceive(unsigned char deviceAddr, unsigned char length, unsigned char *data)
{
	 unsigned char ret;
	 
	 ret = i2c_start(deviceAddr+I2C_READ);       // set device address and write mode  
   if ( ret ) {
      /* failed to issue start condition, possibly no device found */
      i2c_stop(); //release bus, as this has failed
      DC('F'); DC('a'); DC('i'); DC('l'); DC('e'); DC('d'); DC(' '); DC('I'); DC('2'); DC('C');
      DNL();
      return ret;
   } else {   
   	
      // accept receive data and ack it
      while(length > 1)
      {
      	*data++ = i2c_readAck();   
        // decrement length
        length--;
      }
      // accept receive data and nack it (last-byte signal)
      *data++ = i2c_readNak();   
      i2c_stop();  
      return I2C_OK;
		}
}