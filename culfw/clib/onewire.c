#include "board.h"
#ifdef HAS_ONEWIRE
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/boot.h>
#include <math.h>

#include "fncollection.h"
#include "stringfunc.h"
#include "display.h"
#include "delay.h"

#include "onewire.h"
#include "i2cmaster.h"

static unsigned char dscrc_table[] = {
        0, 94,188,226, 97, 63,221,131,194,156,126, 32,163,253, 31, 65,
      157,195, 33,127,252,162, 64, 30, 95,  1,227,189, 62, 96,130,220,
       35,125,159,193, 66, 28,254,160,225,191, 93,  3,128,222, 60, 98,
      190,224,  2, 92,223,129, 99, 61,124, 34,192,158, 29, 67,161,255,
       70, 24,250,164, 39,121,155,197,132,218, 56,102,229,187, 89,  7,
      219,133,103, 57,186,228,  6, 88, 25, 71,165,251,120, 38,196,154,
      101, 59,217,135,  4, 90,184,230,167,249, 27, 69,198,152,122, 36,
      248,166, 68, 26,153,199, 37,123, 58,100,134,216, 91,  5,231,185,
      140,210, 48,110,237,179, 81, 15, 78, 16,242,172, 47,113,147,205,
       17, 79,173,243,112, 46,204,146,211,141,111, 49,178,236, 14, 80,
      175,241, 19, 77,206,144,114, 44,109, 51,209,143, 12, 82,176,238,
       50,108,142,208, 83, 13,239,177,240,174, 76, 18,145,207, 45,115,
      202,148,118, 40,171,245, 23, 73,  8, 86,180,234,105, 55,213,139,
       87,  9,235,181, 54,104,138,212,149,203, 41,119,244,170, 72, 22,
      233,183, 85, 11,136,214, 52,106, 43,117,151,201, 74, 20,246,168,
      116, 42,200,150, 21, 75,169,247,182,232, 10, 84,215,137,107, 53};

// Buffer for OnwWire Bus Devices
unsigned char ROM_CODES[HAS_ONEWIRE * 8];
int onewire_connecteddevices;
int onewire_conversionrunning;
int onewire_allconversionsrunning;
int onewire_allconversiontimer;
int onewire_hmsemulation;
int onewire_hmsemulationstate;
int onewire_hmsemulationtimer;
int onewire_hmsemulationinterval;
int onewire_hmsemulationdevicecounter;

// Search states for the onewire_Search
int LastDiscrepancy;
int LastFamilyDiscrepancy;
int LastDeviceFlag;
int DeviceCounter;
unsigned char crc8;


void 
onewire_Init(void) 
{
    onewire_hmsemulationinterval = 120;     //Resets also the HMS-Emulation Interval to xx Seconds
    onewire_hmsemulation = 0;                           //Resets also HMS-Emulation to OFF
    onewire_allconversionsrunning = 0;
    onewire_allconversiontimer = 0;
    onewire_conversionrunning = 0;
    onewire_hmsemulationstate = 0;
    onewire_hmsemulationtimer = onewire_hmsemulationinterval;
    onewire_SearchReset();
    if (ds2482Init()) {
    if (onewire_Reset() == 1) 
         onewire_FullSearch();
 }
}

void 
onewire_HsecTask(void) 
{   
    if (onewire_conversionrunning) {
        if (onewire_ReadByte() & DS2482_STATUS_DIR)
            onewire_conversionrunning = 0;
    }
    if (onewire_allconversionsrunning) {
        onewire_allconversiontimer--;
        if (onewire_allconversiontimer == 0)
            onewire_allconversionsrunning = 0;
    }
    if (onewire_hmsemulation && !onewire_hmsemulationtimer) { //HMS-Emulation & timer has exipered
        if (onewire_hmsemulationstate == 0) {                                       //First step for HMS Emulaion: Start all conversions
            // Reset the bus
            onewire_Reset();
            //Skip ROMs to start ALL conversions
        onewire_WriteByte(0xCC); // Skip ROMs
            //Write Conversion Command
        onewire_WriteByte(0x44); // Start Conversion
        // wait for DS2482 to finish
            onewire_BusyWait();
            //Set Marker for Conversion running
            onewire_allconversionsrunning = 1;
            onewire_allconversiontimer = 7;                                             //It takes ~750ms to finish all conversions: 125msec timer * 6
            onewire_hmsemulationstate = 1;
            onewire_hmsemulationdevicecounter = 0;
        } else if (onewire_hmsemulationstate == 1) {                                // Conversions have been started already
                if (!onewire_allconversionsrunning) {                                       // Are all conversions already done ?
                    //Start to read out the things, but one-by-one in turns
                    if (onewire_hmsemulationdevicecounter < onewire_connecteddevices) {     // go through all connected devices
                        //Chek if this device is a Temp-Sensor  //Family: 28h - DS18B20; 10h - DS18S20; 22h - DS1822
                        if ((ROM_CODES[onewire_hmsemulationdevicecounter*8] == 40) || (ROM_CODES[onewire_hmsemulationdevicecounter*8] == 34) || (ROM_CODES[onewire_hmsemulationdevicecounter*8] == 16)) {  
                            //Match ROM
                            onewire_Reset();
		                        onewire_WriteByte(0x55); // Start RomMatch
		                        onewire_WriteByte(ROM_CODES[onewire_hmsemulationdevicecounter*8]);
		                        onewire_WriteByte(ROM_CODES[onewire_hmsemulationdevicecounter*8 + 1]);
		                        onewire_WriteByte(ROM_CODES[onewire_hmsemulationdevicecounter*8 + 2]);
		                        onewire_WriteByte(ROM_CODES[onewire_hmsemulationdevicecounter*8 + 3]);
		                        onewire_WriteByte(ROM_CODES[onewire_hmsemulationdevicecounter*8 + 4]);
		                        onewire_WriteByte(ROM_CODES[onewire_hmsemulationdevicecounter*8 + 5]);
		                        onewire_WriteByte(ROM_CODES[onewire_hmsemulationdevicecounter*8 + 6]);
		                        onewire_WriteByte(ROM_CODES[onewire_hmsemulationdevicecounter*8 + 7]);
		                        onewire_hmsemulationstate = 2;
                        } else
                            onewire_hmsemulationdevicecounter++;
                    } else {        //We are done with emulation Reset Timer + Vars
                        onewire_hmsemulationtimer= onewire_hmsemulationinterval;
                        onewire_hmsemulationstate = 0;
                        onewire_hmsemulationdevicecounter = 0;
                    }
                }
        } else if (onewire_hmsemulationstate == 2) {                            //MatchRom for selected Temp-Sensor has been done
            //Read Scratchpad for selected Device (only Temp-Sensors)    
            if ( (ROM_CODES[onewire_hmsemulationdevicecounter*8] == 40) || (ROM_CODES[onewire_hmsemulationdevicecounter*8] == 34) || (ROM_CODES[onewire_hmsemulationdevicecounter*8] == 16)) {
            	    
            	char 	get[10]; 
              int  	temp = 0;;
              char 	sign = '0';
              double 	tempf = 0.0;
                
              onewire_WriteByte(0xBE); // Read Scratch Pad
              for (int k=0;k<9;k++){get[k]=onewire_ReadByte();}
            
					    temp = (get[1] << 8) | get[0];
						  if ( (ROM_CODES[onewire_hmsemulationdevicecounter*8] == 40) || (ROM_CODES[onewire_hmsemulationdevicecounter*8] == 34) ) { 		//DS18B20 and DS1822
						    tempf = ((double)(temp))/16.0;
						  } else {// if (ROM_CODES[onewire_hmsemulationdevicecounter*8] == 16) { //DS18S20
								double count_per_c  = (double) get[7];
								double count_remain = (double) get[6];
								temp = temp / 2;
								tempf = (double)temp - 0.25 + ((count_per_c - count_remain)/ count_per_c);
							}
							temp = rndup(tempf);
							if (temp < 0 )
							{
								sign = '8';
								temp*=-1;
							}
							DC('H');
							DH2(ROM_CODES[onewire_hmsemulationdevicecounter*8+2]);
							DH2(ROM_CODES[onewire_hmsemulationdevicecounter*8+1]);
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
      	onewire_hmsemulationdevicecounter++;                                        //Done with this sensor, go to the next
      	onewire_hmsemulationstate = 1;
    	}
  	}
}

int
rndup(double n)		//round up a float type and show one decimal place
{
      double t = 0.0;
      n *= 10.0;
      t=n-floor(n);
     
      if (t>=0.5555)  {
	      n = ceil(n);;
      }
      else {
      	n = floor(n);
      }
      return (int)n;
}      

void
onewire_SecTask(void)
{
    if (onewire_hmsemulation) {
        if (onewire_hmsemulationtimer)
            onewire_hmsemulationtimer--;
    }
}

int
onewire_Reset(void)
{
    unsigned char status;
    //Make sure that any Conversion is stopped & ignored
    onewire_conversionrunning = 0;
    // send 1-Wire bus reset command
    ds2482SendCmd(DS2482_CMD_1WRS);
    // wait for bus reset to finish, and get status
    status = onewire_BusyWait();
    // return Short Detected
    if (status & DS2482_STATUS_SD)
        return 2;
    // return state of the presence bit
    if (status & DS2482_STATUS_PPD) {
        return 1;
    } else {
        return 0;   
    }
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
onewire_WriteBit(unsigned char data)
{
    // wait for DS2482 to be ready
    onewire_BusyWait();
    // send 1WSB command
    ds2482SendCmdArg(DS2482_CMD_1WSB, data?0x80:0x00);
    //Wait for Bus to finish
    onewire_BusyWait();
}


void 
onewire_WriteByte(unsigned char data)
{
    // wait for DS2482 to be ready
    onewire_BusyWait();
    // send 1WWB command
    ds2482SendCmdArg(DS2482_CMD_1WWB, data);    
    // Wait to finish;
    onewire_BusyWait();
}

unsigned char
onewire_ReadByte(void)
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

unsigned char
onewire_ReadBit(void)
{
    //Activate Bit Operation and make TimeSlot
    onewire_WriteBit(1);
    //Read Status Register for Answer
    if (onewire_BusyWait() & DS2482_STATUS_SBR)
        return 1;
    else
        return 0;
}

void 
onewire_MatchRom(unsigned char* romaddress)
{
  	//Start
  	onewire_Reset();
  	onewire_WriteByte(0x55); // Start RomMatch
  	onewire_WriteByte(romaddress[7]);
  	onewire_WriteByte(romaddress[6]);
  	onewire_WriteByte(romaddress[5]);
  	onewire_WriteByte(romaddress[4]);
  	onewire_WriteByte(romaddress[3]);
  	onewire_WriteByte(romaddress[2]);
  	onewire_WriteByte(romaddress[1]);
  	onewire_WriteByte(romaddress[0]);
}

void 
onewire_StartConversion(void)
{
  	// wait for DS2482 to be ready
  	onewire_BusyWait();
  	//Write Conversion Command
  	onewire_WriteByte(0x44); // Start Conversion
  	// wait for DS2482 to finish
  	onewire_BusyWait();
  	//Set Marker for Conversion running
  	onewire_conversionrunning = 1;
}

int
onewire_CheckConversionRunning(void)
{
    //Check Marker for Conversion running
    if (onewire_conversionrunning)
        return 1;
    else
        return 0;   
}

int
onewire_CheckAllConversionsRunning(void)
{
    //Check Marker for Conversion running
    if (onewire_allconversionsrunning)
        return 1;
    else
        return 0;   
}


//--------------------------------------------------------------------------
// Performs a full OneWire search
//        until all devices have been found, or MAX_DEVICES is reached
//
void onewire_FullSearch(void)
{
    //Start Search
    onewire_SearchReset();
  if (onewire_Search()) {
        do
        {
            DC('R');
            DC(':');
            DH2(ROM_CODES[DeviceCounter*8 + 7]);
            DH2(ROM_CODES[DeviceCounter*8 + 6]);
            DH2(ROM_CODES[DeviceCounter*8 + 5]);
            DH2(ROM_CODES[DeviceCounter*8 + 4]);
            DH2(ROM_CODES[DeviceCounter*8 + 3]);
            DH2(ROM_CODES[DeviceCounter*8 + 2]);
            DH2(ROM_CODES[DeviceCounter*8 + 1]);
            DH2(ROM_CODES[DeviceCounter*8 + 0]);
            DeviceCounter++;
        DNL();
        }
        while (onewire_Search() && (DeviceCounter < (HAS_ONEWIRE)));
    }
    onewire_connecteddevices = DeviceCounter;
    DC('D'); DC(':');DU(onewire_connecteddevices, 2);DNL();
}
//--------------------------------------------------------------------------
// Resets the OneWire Search Function, so that the next search will start
//        with the first device again
void onewire_SearchReset(void)
{
    int i;
  // reset the search state
  LastDiscrepancy = 0;
  LastDeviceFlag = DS2482_FALSE;
  LastFamilyDiscrepancy = 0;
  DeviceCounter = 0;
  onewire_connecteddevices = 0;
  
  for (i=0;i<(HAS_ONEWIRE * 8);i++) {
    ROM_CODES[i] = 0;
  }
}
//--------------------------------------------------------------------------
// The 'onewire_Search' function does a general search. This function
// continues from the previous search state. The search state
// can be reset by using the 'onewire_SearchReset' function.
//
// Returns:   DS2482_TRUE (1) : when a 1-Wire device was found and its
//                              Serial Number placed in the global ROM
//            DS2482_FALSE (0): when no new device was found.  Either the
//                              last search was the last device or there
//                              are no devices on the 1-Wire Net.
//
int onewire_Search(void)
{
   int id_bit_number;
   int last_zero, rom_byte_number, search_result;
   unsigned char id_bit, cmp_id_bit;
   unsigned char rom_byte_mask;
   unsigned char search_direction;

   // initialize for search
   id_bit_number = 1;
   last_zero = 0;
   rom_byte_number = 0;
   rom_byte_mask = 1;
   search_direction = 0;
   search_result = DS2482_TRUE;
   crc8 = 0;

   // if the last call was not the last one
   if (!LastDeviceFlag)
   {
      // 1-Wire reset
      if (!onewire_Reset())     
      {
         // reset the search
         LastDiscrepancy = 0;
         LastDeviceFlag = DS2482_FALSE;
         LastFamilyDiscrepancy = 0;
         return DS2482_FALSE;
      }

      // issue the search command
      onewire_WriteByte(0xF0);

      // loop to do the search
 
      do
      {
         // read a bit and its complement
         id_bit = onewire_ReadBit();
         cmp_id_bit = onewire_ReadBit();

         // check for no devices on 1-wire
         if ((id_bit == 1) && (cmp_id_bit == 1))
            break;
         else
         {
            // all devices coupled have 0 or 1
            if (id_bit != cmp_id_bit)
               search_direction = id_bit;  // bit write value for search
            else
            {
               // if this discrepancy if before the Last Discrepancy
               // on a previous next then pick the same as last time
               if (id_bit_number < LastDiscrepancy)
                  if (DeviceCounter > 0)
                    search_direction = ((ROM_CODES[(DeviceCounter-1)*8 + rom_byte_number] & rom_byte_mask) > 0);
                  else 
                    search_direction = 0 ;
               else
                  // if equal to last pick 1, if not then pick 0
                  search_direction = (id_bit_number == LastDiscrepancy);

               // if 0 was picked then record its position in LastZero
               if (search_direction == 0)
               {
                  last_zero = id_bit_number;

                  // check for Last discrepancy in family
                  if (last_zero < 9)
                     LastFamilyDiscrepancy = last_zero;
               }
            }

            // set or clear the bit in the ROM byte rom_byte_number
            // with mask rom_byte_mask
            if (search_direction == 1)
              ROM_CODES[DeviceCounter*8 + rom_byte_number] |= rom_byte_mask;
            else
              ROM_CODES[DeviceCounter*8 + rom_byte_number] &= ~rom_byte_mask;

            // serial number search direction write bit
            onewire_WriteBit(search_direction);

            // increment the byte counter id_bit_number
            // and shift the mask rom_byte_mask
            id_bit_number++;
            rom_byte_mask <<= 1;

            // if the mask is 0 then go to new SerialNum byte rom_byte_number and reset mask
            if (rom_byte_mask == 0)
            {
                docrc8(ROM_CODES[DeviceCounter*8 + rom_byte_number]);  // accumulate the CRC
                rom_byte_number++;
                rom_byte_mask = 1;
            }
         }
      }
      while(rom_byte_number < 8);  // loop until through all ROM bytes 0-7

      // if the search was successful then
      if (!((id_bit_number < 65) || (crc8 != 0)))
      {
         // search successful so set LastDiscrepancy,LastDeviceFlag
         // search_result
         LastDiscrepancy = last_zero;

         // check for last device
         if (LastDiscrepancy == 0)
            LastDeviceFlag = DS2482_TRUE;
                 // check for last family group
                 if (LastFamilyDiscrepancy == LastDiscrepancy)
                      LastFamilyDiscrepancy = 0;

         search_result = DS2482_TRUE;
      }
   }

   // if no device found then reset counters so next
   // 'search' will be like a first

   if (!search_result || (!ROM_CODES[DeviceCounter*8]))
   {
      LastDiscrepancy = 0;
      LastDeviceFlag = DS2482_FALSE;
      LastFamilyDiscrepancy = 0;
      search_result = DS2482_FALSE;
   }
   return search_result;
}

//--------------------------------------------------------------------------
// Calculate the CRC8 of the byte value provided with the current 
// global 'crc8' value. 
// Returns current global crc8 value
//
unsigned char docrc8(unsigned char value)
{
   // See Application Note 27
   
   // TEST BUILD
   crc8 = dscrc_table[crc8 ^ value];
   return crc8;
}


//--------------------------------------------------------------------------
// Reads the RomCodes of found Devices from the RAM-Table 
// Output: index:aaaaaaaaaaaaaaaa     example: 1:D500123456789a28
//
void 
onewire_ReadROMCodes(void)
{
  int i, n;

    //We know, that we have done at least an initial Full OneWire Search
    for (i=0;i<onewire_connecteddevices;i++)
    {
        DU(i+1,0);DC(':');
      for (n=8;n>0;n--){
        DH2(ROM_CODES[i*8+(n-1)]);
      }
      DNL();
    }
}


void 
onewire_ReadTemperature(void)
{
  char get[10];
  int k;
  int temp;
  
  onewire_WriteByte(0xBE); // Read Scratch Pad
  for (k=0;k<9;k++){get[k]=onewire_ReadByte();}
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



//--------------------------------------------------------------------------
// Main OneWire User-Function 
// interpretes the User-Command and calls right function
//
void
onewire_func(char *in)
{
  unsigned char byteword;
  unsigned char romaddress[8];

  if(in[1] == 'i') {
        onewire_Init();
        DC('O'); DC('K');
    DNL();
  } else if(in[1] == 'R') {
          if (in[2] == 'm') {
            if (ds2482Reset()) {
                DC('O'); DC('K');
                DNL();
            } else {
            DC('F'); DC('a'); DC('i'); DC('l'); DC('e'); DC('d'); DC(' '); DC('I'); DC('2'); DC('C');
            DNL();
            }
      } else if(in[2] == 'b') { 
        DC('O'); DC('K'); DC(':'); DU(onewire_Reset(), 0);
            DNL();      
      }
  } else if(in[1] == 'c') {                         //Read RomCodes from Tabel
            onewire_ReadROMCodes();     
  } else if(in[1] == 'r') {                         //Read Command
          if (in[2] == 'b') {                               // -b Read Bit from Bus
            DU(onewire_ReadBit(), 0);
            DNL();
        } else if(in[2] == 'B') {               // -B Read Byte from Bus
            DH2(onewire_ReadByte());
            DNL();
        } else if(in[2] == 'S') {               // -S Read Full ScratchPad from Device
        }
  } else if(in[1] == 'w') {                         //Write Command
          if (in[2] == 'b') {                               // -b Write Bit to Bus
            fromhex (in+3, &byteword, 1);
            if (byteword)
                byteword = 1;
            else
                byteword = 0;
          //DC('W'); DC('b'); DC(':');DH2(byteword); DNL();         // Debug output of Bit to write
            onewire_WriteBit(byteword);
        } else if(in[2] == 'B') {               // -B Write Byte to Bus
            fromhex (in+3, &byteword, 1);
          //DC('W'); DC('B'); DC(':');DH2(byteword); DNL();         // Debug output of Byte to write
            onewire_WriteByte(byteword);
        } 
  } else if(in[1] == 'm') {
        fromhex (in+2, romaddress, 8);
            DC('m');DC(':');DH2(romaddress[0]);DH2(romaddress[1]);DH2(romaddress[2]);DH2(romaddress[3]);DH2(romaddress[4]);DH2(romaddress[5]);DH2(romaddress[6]);DH2(romaddress[7]);DNL();
            onewire_MatchRom(romaddress);
  } else if(in[1] == 'H') {
        if (in[2] == 'o') {                 
            onewire_allconversionsrunning = 0;
                onewire_allconversiontimer = 0;
                onewire_hmsemulationstate = 0;
                onewire_hmsemulationtimer = onewire_hmsemulationinterval;
            if (onewire_hmsemulation) {
                    onewire_hmsemulation = 0;
                    DC('O');DC('F');DC('F');DNL();
                } else {
                    onewire_hmsemulation = 1;
                    //DO an initial Conversion, as the sensors need a first round
                    // Reset the bus 
                    onewire_Reset();
                    //Skip ROMs to start ALL conversions
                onewire_WriteByte(0xCC); // Skip ROMs
                    //Write Conversion Command
                onewire_WriteByte(0x44); // Start Conversion
                // wait for DS2482 to finish
                    onewire_BusyWait();
                    DC('O');DC('N');DNL();
                }
            } else  if (in[2] == 't') {                 
                fromdec (in+3, (uint8_t *)&onewire_hmsemulationinterval);
            }
  } else if(in[1] == 't') {
        onewire_ReadTemperature();      
  } else if(in[1] == 'C') {
        if (in[2] == 's') {                 
            onewire_StartConversion();      
        } else if(in[2] == 'r') {
            DU(onewire_CheckConversionRunning(),0); DNL();
        } else if(in[2] == 'a') {
            if (onewire_hmsemulation) { 
                DU(onewire_CheckAllConversionsRunning(),0); DNL();
            } else {
                DC('0');DNL();
            }
        }
  } else if(in[1] == 'f') {
            //onewire_FullSearch();     
              onewire_FullSearch();
  }   
}


//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
// DS2482 Command functions
// These functions are used to address the DS2482 directly (via I2C)
// The idea is to send Commands and Commands with arguments

int
ds2482Init(void)
{
    unsigned char ret;
  //we know, that I2C is already initialized and running
   ret = i2c_start(DS2482_I2C_ADDR+I2C_WRITE);       // set device address and write mode  
   if ( ret ) {
      /* failed to issue start condition, possibly no device found */
      i2c_stop(); //release bus, as this has failed
      return 0;
   } else {
        i2c_stop(); //release bus, as this was just an initialization
            return 1;
  }
}

int
ds2482Reset(void)
{
    unsigned char ret;
    ret = ds2482SendCmd(DS2482_CMD_DRST);
    if (ret == I2C_OK) {
#ifdef OW_SPU
        return (ds2482SendCmdArg(DS2482_CMD_WCFG, DS2482_CFG_SPU) == I2C_OK) ? 1 : 0;
#else
        return 1;
#endif
    } else {
        return 0;
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
  //    rprintf("Cmd=0x%x  Status=0x%x\r\n", cmd, data);
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
    //  rprintf("Cmd=0x%x  Arg=0x%x  Status=0x%x\r\n", cmd, arg, data[0]);

    return (I2C_OK);
}

//--------------------------------------------------------------------------
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

#endif
