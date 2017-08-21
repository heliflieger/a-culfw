/* 
 * ------------------------------------------------------------------------------------------------------------------------------------------------------------
 * This is my first trial to send to Kopp Free Control Units via CUL CCD Module
 * 
 * Commands from FHEM are transferred to this kopp modul via variable in.																					
 * Function kopp_fc_func() is called if the first parameter out of FHEM is "K"																			
 * Transmission of one remote block is started if parameter in[1] = "t", any other parameter will lead to an error message (not sure whether FHEM will display it)
 * Remaining parameters will contain the "Key code"	(Key which has been pressed on the remote control), 
 *   if an offset of 0x80 is added this will signal a long key pressure
 *   afterward the code of the remote transmitter itself will be transmitted
 *   finally the time (in msec, max 99999) will be transmitted (to define the length of a "long key pressure". This needs to be defined also for short key pressures (key code < ox8x)
 *   although this value will not be used for short key pressures
 *																					  
 *	 Pameter String definition (transmitt): 	ktCCXXXXYYTTTTTP
 *   "k":		command will be routed to Kopp-fc
 *   "t" or "s"	Command is "transmitt"; "t" will send "key off code" if "key code" >= 0x80, "s" will not send "key off code" at all
 *   "CC":		Key code (Hex, Byte)
 *   "XXXX":	Transmitter code 1 (Hex, 2 Byte)
 *   "YY":		Transmitter code 2 (Hex, Byte)
 *   "TTTTT":	Key is pressed for TTTTT msec (KeyOffTime) (Dez)
 *	 "J"or"N":	Print Output messages (should not be done whithin Call from FHEM, CUL/CCD will provide error messages in logfile)
 *   
 *   Example of parameter string:  	kt30F96E0110000J   (in this example the time out is 10sec but will not be used because its a short key pressure)																																			   
 *   Example of parameter string:  	ktB0F96E0105000N   (in this example the time out is 5sec, long key preassure for key code is 0x30(+0x80)																																			   
 *				 
 *	 Pameter String definition (receive): 	krS krE 
 *   "k":		command will be routed to Kopp-fc
 *   "r": 		Command is "receive
 *   "S" or "E" "S" will start Receive mode, "E" will end receive mode
 *   "1" or "2": Optional Parameter for debugging only "1" will show each received block, "2" will show any received bytes, incl. zeros....
 *
 *	 some hints:
 *   =========== 
 *   - it seems, that the Firware sends the information to the display/serial interface (e.g. via DS_P() command) after this routine
 *     was left. That means, that nothing will displayed, when e.g, watchdog timer overruns. 
 *     For debugging issues this is the hell :-(    
 *   - found out, that this routine makes some courious stuff if e.g. strtol() works on wrong number base (10/16/autodetect)									  
 *
 *   - I don't use the KOPP-FC.PM Module yet, just try to use direct communication via CCD/CUL "set CCD raw kt30F96E10000"
 *
 *					
 * Date		   Who				Comment																												   
 * ----------  -------------   	-------------------------------------------------------------------------------------------------------------------------------
 * 2016-04.04  RaspII			Added Debug Information and additional Watchdog handling (no more watchdog resets reported by Feuerdrache)
 * 2016-03-29  RaspII           Generate no error messages anymore for Receive Checksum Errors
 * 2016-02-22  RaspII			Received blocks (HF) are only accepted if blocklength = 7
 * 2015-04-21  RaspII			first version of Receive mode (used rf_mbus as example) implemented, usage only in terminal, yet
 * 2015-02-01  RaspII			Changed Line-Endings to Unix style, changed comment above (this routine will be called from FHEM with "k" (little) and no more "K"
 *                              removed useless lines
 * 2014-12-13  RaspII			set Baudrate to 83 hex (previously 82)
 * 2014-12-13  RaspII			Added Print Option because else FHEM will provide error messages to logfile
 * 2014-12-06  RaspII			Added second command "s" for transmitt on data block only (no "Key off" code will be sent)
 *								Tansmitter Code was not sent, added Transmitter Code 2
 * 2014-08-29  RaspII			Added "Long Key Preasure" (Remote control key pressed for x msec) 
 * 2014-08-08  RaspII			Now transmitting one block is working with Kopp Free Control protocol, key Code from Input parameter (1 character)
 * 2014-08-01  RaspII			first Version
 *
 * 
 * ------------------------------------------------------------------------------------------------------------------------------------------------------------
 */

#include <avr/io.h>                     // for _BV, bit_is_set
#include <stdint.h>                     // for uint8_t, uint16_t, uint32_t

#include "board.h"                      // for CC1100_CS_DDR, etc
#include "led.h"                        // for SET_BIT
#include "stringfunc.h"                 // for fromhex
#ifdef HAS_KOPP_FC
#include <avr/pgmspace.h>               // for PSTR, PROGMEM, __LPM
#include <avr/wdt.h>                    // for wdt_reset
#include <stdlib.h>                     // for strtol, NULL
#include <string.h>                     // for strlen, strcpy, strncpy

#include "cc1100.h"                     // for cc1100_sendbyte, etc
#include "clock.h"                      // for ticks
#include "delay.h"                      // for my_delay_us, my_delay_ms
#include "display.h"                    // for DS_P, DH, DH2, DU, DNL, DS
#include "fband.h"                      // for checkFrequency
#include "fncollection.h"               // for EE_CC1100_CFG_SIZE, erb, etc
#include "kopp-fc.h"
#include "rf_mode.h"

#ifdef USE_HAL
#include "hal.h"
#endif

void kopp_fc_sendraw(uint8_t* buf, int longPreamble);
void kopp_fc_sendAck(uint8_t* enc);
void kopp_fc_handleAutoAck(uint8_t* enc);

#define MCSM1_Init  		0x0C    			// CCA_Mode: Always; Receive: stayin receive mode after package reception; Transmitt:  go idle after finisching package transmission
#define MCSM1_TXOFF_Mask  	0xFC    			// MCSM1 And Mask for Transmitt Off behavior
#define MCSM1_TXOFF_Idle  	0x00    			// Transmitt: Transmitter goes idle after finisching package transmission
#define MCSM1_TXOFF_TX  	0x02    			// Transmitt: Transmitter stays active and sends preamble again after finisching package transmission
#define TicksToMsec			8					// 1 tick = 8 msec (should be defined in a global modul 
												// to display timeout in msec: timeout[ms] = timeout[ticks]*TicksToMsec 
												// to convert ms in ticks: timeout[ticks] = timeout[ms]/TicksToMsec 
#define Watchdogtime_Kopp   1000/TicksToMsec	// Watchdog will be reset after 1 sec (if we wait for long times)  


uint8_t kopp_fc_on = 0;
uint8_t kopp_fc_rx_on = 0;
uint8_t kopp_fc_tx_on = 0;
uint8_t kopp_fc_debug_lvl = 0;					// Debug Level for Receive Mode


uint8_t blkctr;
char ErrorMSG[30] = "ok";
uint32_t BlockStartTime;											// Block Start Time is global
char printon[1] = "N";												// Sollen wir Daten ausgeben (Zeitstempel etc), default: nein
uint8_t lastrecblk[MAX_kopp_fc_MSG+1]={0};							// Last Receive Block (to identify different command/key preasure, each command was send 13x, +1 because blocklen is included)  
 

// Kopp Free-Control Inititalisieren
// ==================================

const PROGMEM const uint8_t CC1100_Kopp_CFG[EE_CC1100_CFG_SIZE] = {
//  CC1101 Register Initialisation (see CC1101 Page 70ff and 62ff)
//  Data   				Adr  Reg.Name RESET STUDIO COMMENT
// ======  				==== ======== ===== ====== =================================================================================================================================
	0x01, 				// 00  IOCFG2   *29   *0B    GDO2_CFG=1: GDO2 Asserts when the RX Fifo is filled or above the RX FIFO threshold or the end of package is reached. Deasserted if the Fifo is empty
	0x2E, 				// 01  IOCFG1    2E    2E    no change yet
	0x46,				// 02  IOCFG0   *3F   *0C    GDO0_CFG=2: Associated to the TX FIFO: Asserts when the TX FIFO is filled at or above the TX FIFO threshold. 
						//                   		 De-asserts when the TXFIFO is below the same threshold.   
	0x04, 				// 03  FIFOTHR   04   *47	 RX Fifo Threshold = 20 Bytes
	0xAA, 				// 04  SYNC1     D3    D3    Sync High Byte = AA (assumption: High Byte is first send sync byte)
	0x54, 				// 05  SYNC0     91    91    Sync Low  Byte = 54 (AA 54 should work with CC1101 as Sync word)
	MAX_kopp_fc_MSG, 	// 06  PKTLEN   *FF    3D    Package length for Kopp 15 Bytes (incl. Cks+Zeros, because handled as data because no standard CC1101 checksum)
	0xE0, 				// 07  PKTCTRL1  04    04    Preamble quality is maximum(7), No Auto RX Fifo Flush, No Status Bytes will be send, No Address check
	0x00, 				// 08  PKTCTRL0 *45    32    Data whitening off,  Rx and Tx Fifo, CRC disabled, Fixed package length
	0x00, 				// 09  ADDR      00    00    Device Adress (Address filter not used)
	0x00, 				// 0A  CHANNR    00    00    Channel Number (added to Base Frequency) is not used
	0x06, 				// 0B  FSCTRL1  *0F    06    152,34375 kHz IF Frquency (##Claus: to be adjusted for Kopp, later if RX is used)
	0x00, 				// 0C  FSCTRL0   00    00    Frequency Offset = 0
	0x21, 				// 0D  FREQ2    *1E    21    FREQ[23..0] = f(carrier)/f(XOSC)*2^16  -> 868,3Mhz / 26Mhz * 2^16 = 2188650 dez = 21656A hex  (f(XOSC)=26 Mhz)
	0x65, 				// 0E  FREQ1    *C4    65    s.o.
	0x6A, 				// 0F  FREQ0    *EC    e8    s.o.
	0x97, 				// 10  MDMCFG4  *8C    55    bWidth 162,5 kHz   (Kopp 50 Khz!, but does not work))
	0x83, 				// 11  MDMCFG3  *22   *43    Drate: 4785,5 Baud   (Kopp: 4789 Baud, measured value !! may be increase by 1 is needed (83) because value should be 4800) doesn't work better with 81/83 )
	0x16, 				// 12  MDMCFG2  *02   *B0    DC Blocking filter enabled, GFSK modulation (Kopp uses FSK, do not know whether 2-FSK, GFSK odr 4-FSK), 
						//                           manchester en-decoding disabled, 16 sync bits to match+carrier-sense above threshold
	0x63, 				// 13  MDMCFG1  *22    23    Error Correction disabled, min 16 preamble bytes, Channel spacing = 350 khz
	0xb9, 				// 14  MDMCFG0  *F8    b9    Channel spacing 350kHz  (Copied from somfy, do not know if ok ) 
	0x47, 				// 15  DEVIATN  *47    00    frequency deviation = 47,607 khz (default, do not know if right, for RFM12b I used 45khz)
	0x07, 				// 16  MCSM2     07    07    
	MCSM1_Init, 		// 17  MCSM1     30    30    see above
	0x29, 				// 18  MCSM0    *04    18    Calibration after RX/TX -> IDLE, PO_Timeout=2 ##Claus 0x01: Oszillator always on for testing
	0x36, 				// 19  FOCCFG   *36    14
	0x6C, 				// 1A  BSCFG     6C    6C
	0x07, 				// 1B  AGCCTRL2 *03   *03    42 dB instead of 33dB
	0x40, 				// 1C  AGCCTRL1 *40   *40
	0x91, 				// 1D  AGCCTRL0 *91   *92    
	0x87, 				// 1E  WOREVT1   87    87
	0x6B, 				// 1F  WOREVT0   6B    6B
	0xF8, 				// 20  WORCTRL   F8    F8
	0x56, 				// 21  FREND1    56    56
	0x11, 				// 22  FREND0   *16    17   0x11 for no PA ramping (before 16, this was the reason why transmission didn't run)
	0xE9, 				// 23  FSCAL3   *A9    E9   as calculated by Smart RF Studio
	0x2A, 				// 24  FSCAL2   *0A    2A   as calculated by Smart RF Studio
	0x00, 				// 25  FSCAL1    20    00   as calculated by Smart RF Studio
	0x1F, 				// 26  FSCAL0    0D    1F   as calculated by Smart RF Studio
	0x41, 				// 27  RCCTRL1   41    41
	0x00, 				// 28  RCCTRL0   00    00
};

// static uint8_t autoAckAddr[3] = {0, 0, 0};
// static uint8_t fakeWallThermostatAddr[3] = {0, 0, 0};
// static uint32_t lastSendingTicks = 0;

void
kopp_fc_init(void)
{
#ifdef USE_HAL
  hal_CC_GDO_init(0,INIT_MODE_IN_CS_IN);
  hal_enable_CC_GDOin_int(0,FALSE); // disable INT - we'll poll...
#else
  EIMSK &= ~_BV(CC1100_INT);                 	// disable INT - we'll poll...
  SET_BIT( CC1100_CS_DDR, CC1100_CS_PIN );   	// CS as output
#endif

// Toggle chip select signal (why?)
  CC1100_DEASSERT;                            	// Chip Select InActiv
  my_delay_us(30);
  CC1100_ASSERT;								// Chip Select Activ
  my_delay_us(30);
  CC1100_DEASSERT;								// Chip Select InActiv
  my_delay_us(45);

  ccStrobe( CC1100_SRES );                   	// Send SRES command (Reset CC110x)
  my_delay_us(100);


// load configuration (CC1100_Kopp_CFG[EE_CC1100_CFG_SIZE])
    CC1100_ASSERT;								// Chip Select Activ
	 cc1100_sendbyte( 0 | CC1100_WRITE_BURST );
	 for(uint8_t i = 0; i < EE_CC1100_CFG_SIZE; i++) 
	 {
	  cc1100_sendbyte(__LPM(CC1100_Kopp_CFG+i));
	 } 
	CC1100_DEASSERT;							// Chip Select InActiv
  
// If I don't missunderstand the code, in module cc1100.c the pa table is defined as 
// 00 and C2 what means power off and max. power.
// so following code (setup PA table) is not needed ?
// did a trial, but does not work


// setup PA table (-> Remove as soon as transmitting ok?), table see cc1100.c
// this initializes the PA table with the table defined at EE_Prom
// which table will be taken depends on command "x00 .... x09"
// x00 means -10dbm pa ramping
// x09 means +10dBm no pa ramping (see cc1100.c) and commandref.html

#ifdef PrintOn                                                                        //
     DS_P(PSTR("PA Table values: "));
#endif

	uint8_t *pa = EE_CC1100_PA;					//  EE_CC1100_PA+32 means max power???
	CC1100_ASSERT;
	cc1100_sendbyte( CC1100_PATABLE | CC1100_WRITE_BURST);

	for (uint8_t i = 0; i < 8; i++) 
	{
#ifdef PrintOn                                                                        //
	DU(erb(pa),0);								// ### Claus, mal sehen was im PA Table steht
    DS_P(PSTR(" "));
#endif
  
    cc1100_sendbyte(erb(pa++));				// fncollection.c "erb()"gibt einen EEPROM Wert zurück
 	}

#ifdef PrintOn 
    DS_P(PSTR("\r\n"));
#endif


 	CC1100_DEASSERT;


// Set CC_ON
	ccStrobe( CC1100_SCAL);						// Calibrate Synthesizer and turn it of. ##Claus brauchen wir das
	my_delay_ms(1);
#ifndef USE_RF_MODE
	cc_on = 1;
#endif

  

  kopp_fc_on = 1;								//##Claus may be not needed in future (Tx Only)
  checkFrequency(); 
}

// kopp_fc_init  E N D
// ======================================================================================================






// Transmitt data block for Kopp Free Control
// ------------------------------------------------------------------------------------------------------------------------------------------
void TransmittKoppBlk(uint8_t sendmsg01[15], uint8_t blkTXcode_i)

{

// Read Blockcounter from Config File Datei (RAMDISK) 
// --------------------------------------------------

   uint8_t blkcks = 0x0;


     
   int count = 0;
   int count2 = 1;
 
   //count2 = 1;														// each block / telegram will be written n times (n = 13 see below)
   sendmsg01[3] = blkctr;                                   		// Write BlockCounter (was incremented) and Transmitt Code (=Transmitter Key) to Array
   sendmsg01[4] = blkTXcode_i;                              		// -----------------------------------------------------------------------------------   

																
																


// Send Block via Transmitter Fifo
// --------------------------------
do {

   ccTX();															// initialize CC110x TX Mode?
   if(cc1100_readReg( CC1100_MARCSTATE ) != MARCSTATE_TX) 			// If Statemachine not MARCSTATE_TX -> error
   { 
    
	DS_P(PSTR("TX_INIT_ERR_"));

    DH2(cc1100_readReg( CC1100_MARCSTATE ));
    DNL();
    kopp_fc_init();
    return;
   }

   BlockStartTime = ticks;                           			// remember Start Time (1 tick=8msec, s.o.)
   blkcks=0xaa;                                                 // Checksumme initialisieren
   count=0;                                                    	//

   CC1100_ASSERT;												// Chip Select Activ
   cc1100_sendbyte(CC1100_WRITE_BURST | CC1100_TXFIFO);			// Burst Mode via Fifo 
                                                               

																// Now send !
   do {	                                                        // ===========
         cc1100_sendbyte(sendmsg01[count]);					    // write date to fifo (fifo is big enough)

          if (count <= 8)                                    	//
		  {
           blkcks=blkcks^sendmsg01[count];                    	//
           if (count==7) sendmsg01[8]=blkcks;               	// write ckecksum to Buffer as soon as calculated
           }                                                    //
																//
  	   count++;                                                	//
 	   } while(count < MAX_kopp_fc_MSG);  		                // Transmitt Byte 0 - AmountofBytes



  CC1100_DEASSERT;												 // Chip Select InActiv

  //Wait for sending to finish (CC1101 will go to RX state automatically

  uint8_t i;
  for(i=0; i< 200;++i) 
  {
//    if( cc1100_readReg( CC1100_MARCSTATE ) == MARCSTATE_RX)		// Claus: After transmission we force idle, always, so RX will not happen
//      break; //now in RX, good
    if( cc1100_readReg( CC1100_MARCSTATE ) != MARCSTATE_TX)	
      break; //neither in RX nor TX, probably some error
    my_delay_ms(1);
  }

// Claus: Test shows i is ~ 0x36, but why so fast??, transmission should need about 25msec (15Bytes*8Bits*4800Bit/sec)
// may be reading status via SPI is also also slow?
 
//  DS_P(PSTR("variable i: "));										// For test only
//  DH((uint16_t) (i),4);											// For test only
//  DS_P(PSTR("\r\n"));												// For test only
 
  count2++;
  
	
 
 } while(count2 <= 13);        	                                // send same message 13x 


blkctr++;  												   			// increase Blockcounter

}

// Transmitt data block for Kopp Free Control - end - 
//===========================================================================================================================================





// Kopp Free-Control Maint task (at least my assumtion it is :-) )
// ===========================================================================================================================================

void
kopp_fc_func(char *in)

{ 
int SingleBlkOnly=0;											// Default:  we will send key off if Keycode >= 0x80
uint32_t LastWatchdog;

uint8_t blkTXcode=0x00; 
uint8_t inhex_dec[kopp_fc_Command_char];						// in_decbin: decimal value of hex commandline
uint8_t hblen = fromhex(in+2, inhex_dec, strlen(in));	
strcpy(ErrorMSG,"ok");		


// If parameter 2 = "t" then  "Transmitt Free Control Telegram" 
SingleBlkOnly=0;												// Default:  we will send key off if Keycode >= 0x80

if((in[1] == 't') || (in[1] == 's')) 


// Transmitt Command
// =================
{
kopp_fc_tx_on = 1;												// Transmitt activated

if (in[15]=='J') printon[0]='Y'; else printon[0]='N'; 		// Sollen wir Daten ausgeben (Zeitstempel etc)

if(in[1] == 's') SingleBlkOnly=1;								// Command = "s", -> If KeyCode > 0x80 we will send no !! Key Off Code

wdt_reset();													// 2016-04-04: ####RaspII Watchdog reset for Feuerdrache to test issues

LastWatchdog=ticks;												// I guess, Watchdog reset was done shortly before 
BlockStartTime=ticks;

// print some status Information
if (printon[0]=='Y')
 {
 DS_P(PSTR("Transmitt\r\n"));
													
 DS_P(PSTR("commandlineparameter: "));
 DS(in);

 DS_P(PSTR("\r\nStringlength:     "));
 DU(strlen(in),0);
 DS_P(PSTR("\r\nNext Character (int) after parameter (should be line end character): "));
 DU((int)in[strlen(in)],0);
 DS_P(PSTR("\r\nAmount of Bytes (Hex) found inside command line parameter:           "));
 DU(hblen,0);
 DS_P(PSTR("\r\n"));
 
   
// following code to check whether ticks uses full 32 bits or will be reset afer 125 ticks
 DS_P(PSTR("Tick Timer:           "));
 DH((uint16_t)(( BlockStartTime>>16) & 0xffff),4);
 DH((uint16_t)(BlockStartTime & 0xffff),4);
 DS_P(PSTR("\r\n"));
 }
	
		
if ((strlen(in)-2 == kopp_fc_Command_char) && (fromhex(in+2, inhex_dec, 2)==2))		

{

kopp_fc_init();										// Init CC11xx for Kopp Free Control protocol	


// Command Line Argument 1 (Hex Char) for "Block Transmitt Code" (uint8_t)
// -----------------------------------------------------------------------          


blkTXcode=inhex_dec[0];																	//Transmitt Code 	                                                                               //


// Extract command Line Argument 4 and convert to Unsingned Integer ("Time till Key off [msec))
// -------------------------------------------------------------------------------------------- 
uint32_t KeyOffTime;
char helpstr[30];
                                                                      //
strncpy(helpstr,in+10,5);
helpstr[5]='\0';																	// Stingende änhängen/einfügen (String muss sauber abgeschlossen werden sonst werden beliebige Zeichen angehängt)
 
KeyOffTime = (uint32_t) strtol(helpstr,NULL,10)/TicksToMsec ;                      // String to Integer, Zahlenbasis 10 (Fehler werden nicht behandelt !! ###Claus später evt. Fehlerbehandlung einbauen
																					// Int wird hier automatisch nach uint8_t convertiert (literatur) 
// Display KeyOfftime
   if (printon[0]=='Y')
   {
   DS_P(PSTR("KeyOffTime:           "));
   DS(helpstr);
   DS_P(PSTR("msec\r\n"));

// Display KeyOffTime in ticks
   DS_P(PSTR("KeyOffTime in ticks:  "));
   DH((uint16_t)((KeyOffTime>>16) & 0xffff),4);
   DH((uint16_t)(KeyOffTime & 0xffff),4);
   DS_P(PSTR("[hex]\r\n"));
   }



// Build transmitt block for "short key pressure"
//----------------------------------------------

// Kurzer Tastendruck, kleiner Handsender, Taste 1
// Texas RF Studio (sniffer) always adds a "0" after first byte received (also with Kopp transmitter, may be a config issue of RF Studio)
uint8_t DefaultKoppMessage[15] =  {0x7, 0xc8, 0xf9, 0x6e, 0x30, 0xcc, 0xf, 0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
//                                |lgth|TrmitterCode|Cnt |KeyNo|- unknown--|TCod2|CkSum|----------6x00---------------|
// Byte:                            0     1     2     3     4     5     6     7    8    9    10   11   12   13   14  

// keep in mind, CC110x will send Preamble (0xAA) automatically !!!!


// We have to add the Transmitter Codes now (is constant for whole transmitt cycle)
   DefaultKoppMessage[1] = inhex_dec[1];                                  		// Transmitter Code 1 High
   DefaultKoppMessage[2] = inhex_dec[2];                             			// Transmitter Code 1 Low
   DefaultKoppMessage[7] = inhex_dec[3];                             			// Transmitter Code 2 
   

// send the whole data block                                                                                         
// --------------------------
// void TransmittKoppBlk(uint8_t[], uint8_t);

 
TransmittKoppBlk(DefaultKoppMessage, blkTXcode);


// If TX Code was >=0x80 -> Long Key preasure, now send 2x Key off
// ------------------------------------------------------------

if ((SingleBlkOnly==0) && (blkTXcode >= 0x80)) 								// Wenn Command = "t" und Keycode >= 0x80
  {                                                        
   blkTXcode = 0xf7;                                                            // load Key Off Code
   uint32_t HelpTime;
 
// need to keep the "Watchdog" quiet, else he will force a reset (we need to send several blocks), leared watchdog needs a retrigger about every 2seconds 

  do {
      if (((ticks-LastWatchdog)&0xffffffff) <= Watchdogtime_Kopp)				// Reset Watchdog any "Watchdogtime_Kopp" seconds 
	  {																			// Should be enough for the remaining code runtime
	   LastWatchdog=ticks;
	   wdt_reset();
	  }
     } while(((ticks-BlockStartTime)&0xffffffff) <= KeyOffTime);        		// wait till Key Off time is gone  (0xfff.... to compensate timer overflow / negative values, not sure wheter needed)

 
 
 // Check Ticks versus BlockStartTime
   HelpTime=ticks;

   if (printon[0]=='Y')
   {
   DS_P(PSTR("Tick Timer:           "));
   DH((uint16_t)((HelpTime>>16) & 0xffff),4);
   DH((uint16_t)(HelpTime & 0xffff),4);
   DS_P(PSTR("[hex]\r\n"));

   DS_P(PSTR("Block Start Time:     "));
   DH((uint16_t)((BlockStartTime>>16) & 0xffff),4);
   DH((uint16_t)(BlockStartTime & 0xffff),4);
   DS_P(PSTR("[hex]\r\n"));

   DS_P(PSTR("KeyOffTime:           "));
   DH((uint16_t)((KeyOffTime>>16) & 0xffff),4);
   DH((uint16_t)(KeyOffTime & 0xffff),4);
   DS_P(PSTR("[hex]\r\n"));

   DS_P(PSTR("ticks-BlockStartTime: "));
   DH((uint16_t)((((ticks-BlockStartTime)&0xffffffff)>>16) & 0xffff),4);
   DH((uint16_t)((ticks-BlockStartTime)&0xffffffff & 0xffff),4);
   DS_P(PSTR("[hex]\r\n"));


   DS_P(PSTR("Block2\r\n"));
   }

   TransmittKoppBlk(DefaultKoppMessage, blkTXcode);                             // Send 1st Key Off Block 

   do {                                                                         // 
      } while(((ticks-BlockStartTime)&0xffffffff) <= 160/TicksToMsec);      	// wait for 160 ms    (160,275 measured with receiver)

   if (printon[0]=='Y')
   {
   DS_P(PSTR("Block3\r\n"));
   }
   
   TransmittKoppBlk(DefaultKoppMessage, blkTXcode);                             // Send 2nd Key Off Block 
   
   } 


   } 
   else 																		// Wrong Transmitt Block length....
   {                          													// Off
    DS_P(PSTR("Kopp Transmitt Command not equal to "));
    DU(kopp_fc_Command_char,0);
    DS_P(PSTR(" Bytes or Keycode is no hex value\r\n"));
    kopp_fc_on = 0;
   }
 
  if (kopp_fc_rx_on == 1) ccRX();												// If transmitt was active before: start CC110x RX Mode again

  kopp_fc_tx_on = 0;															// Transmitt deactivated

  }


// Transmitt Command End 
//----------------------


 
  else if (in[1] == 'r')
  
  
// Receive Command 
// ===============  
  {



  if(in[2] == 'E')

// Receive End/Stop command
// ========================
    {
	kopp_fc_rx_on = 0;
	ccStrobe( CC1100_SIDLE);										// CC11xx goes idle again
	kopp_fc_debug_lvl = 0;	
//	kopp_fc_init();
	DS_P(PSTR("krE-ReceiveEnd\r\n"));
    }


  else if(in[2] == 'S')

// Receive Start command
// ---------------------
// ## RaspII Here I will add a debug level parameter later on to display all received blocks or even all received bytes

	{

     if(in[3] == '1') kopp_fc_debug_lvl = 1;							// define debuglevel
	 else if (in[3] =='2') kopp_fc_debug_lvl = 2;
	 else kopp_fc_debug_lvl = 0;	

	kopp_fc_rx_on = 1;
	kopp_fc_init();													// Init CC11xx for Kopp Free Control protocol (common init subroutine, later we will call this 
																	// subroutine only if basic initialization was not done before)
//    CLEAR_BIT( GDO2_DDR, GDO2_BIT );								// Initialize CC110x Receive  Status Pin (Should be done by system already)

	DS_P(PSTR("krS-ReceiveStart\r\n"));

	
    
	 ccRX();														// initialize CC110x RX Mode?
	 
	 if(cc1100_readReg( CC1100_MARCSTATE ) != MARCSTATE_RX) 		// If Statemachine not MARCSTATE_TX -> error
	  {     														// ------------------------------------------
      DS_P(PSTR("RX_INIT_ERR_MARCSTATE_RX"));
      DH2(cc1100_readReg( CC1100_MARCSTATE ));
      DNL();
      kopp_fc_init();
	  kopp_fc_rx_on = 0;
      return;														// Return könnte weg wenn danach eh nichts mehr kommt
      }
 
 	 
	}
   else
// receive command Sub Command <> "t", "s", "r"
// ============================
  {                         	
    DS_P(PSTR("Kopp Receive SubCommand Unknown\r\n"));
  }

 }

  else

// Sub Command <> "t", "s", "r"
// ============================
  {                          // Off
    DS_P(PSTR("Kopp Command Unknown\r\n"));
    kopp_fc_on = 0;
  }
}

// Main Ganz Ende !!!!! (to be clarified whether code below needed
//===========================================================================================================================================



// Kopp Free-Control Repetive task (also only my assumption how the firmware is structured)
// ===========================================================================================================================================

void kopp_fc_task(void) 

{
uint8_t recbuf[MAX_kopp_fc_MSG+1]={0};								// Receive Buffer (+ 1 Byte because first byte is block length from RXBYTES register)
uint8_t blkcks = 0x0;												// lokal Checksum variable
uint8_t	recckserr= 0;					                    		// default: No checksum Error !
 


 if ((kopp_fc_rx_on != 1)|| (kopp_fc_tx_on == 1)) return;	 		// Nothing to be done if not at Receive Mode or transmitt is ongoing
																	// (transmitt ongoing not needed yet, but needed as soon we send in this task, also

 // RX active, awaiting SYNC

// if (bit_is_set(GDO2_PIN,GDO2_BIT))
#ifdef USE_HAL
  if (hal_CC_Pin_Get(0,CC_Pin_In))
#else
 if (bit_is_set( CC1100_IN_PORT, CC1100_IN_PIN ))					// GDO2=CC100_IN_Port sind so konfirguriert, dass dieser aktiv=high wird
																	// sobald ein kompletter Kopp Block (15 Bytes) oder mehr als 20Bytes im RX-Fifo sind
#endif
 {

  wdt_reset();														// 2016-04-04: ####RaspII Watchdog reset for Feuerdrache to test reset issues

																	// errata #1 does not affect us, because we wait until packet is completely received
//  recbuf[0] = cc1100_readReg( CC1100_RXFIFO ) & 0x7f; 			// read how much data if first byte in Fifo is blklen
  recbuf[0] = cc1100_readReg( CC1100_RXBYTES ) & 0x7f; 			// read how much data in fifo
  
  if(kopp_fc_debug_lvl == 2) 										// <Print Bytes received in Fifo Debuglevel = 2>
  {
   DS_P(PSTR("Bytes in Fifo: "));
   DH2(recbuf[0]);
   DS_P(PSTR("  Received Data: "));
  }
  

  if (recbuf[0]>=MAX_kopp_fc_MSG) recbuf[0] = MAX_kopp_fc_MSG;		// If more bytes than overal block size (incl. Zeros) ignore remaining fifo content

//  DS_P(PSTR("BlkStart  "));										// For test only
//  DH2(recbuf[0]);
//  DU(recbuf[0],0);
 
  blkcks=0xaa;                                                	 	// initialize Checksum 
  recckserr=0;					                    				// default: No checksum Error !
 
  CC1100_ASSERT;
  cc1100_sendbyte( CC1100_READ_BURST | CC1100_RXFIFO );


  for (uint8_t i=0; i<recbuf[0]; i++) 								// Fifo lesen
  {
   recbuf[i+1] = cc1100_sendbyte( 0 );

   if (i < 7+1)                                    					// calculate Checksum for  Receive Bytes 0...7 (=recbuf[1...8])
     {																// ------------------------------------------------------------
      blkcks=blkcks^recbuf[i+1] ;                       			//
     }                                                    			//

   if ((i == 8) && (blkcks!=recbuf[i+1]))            				// Compare Checksum in Buffer to calculated Checksum 
     {																// ------------------------------------------------------------
      recckserr=1;					                    			// Checksum Error !
     }                                                    			//
   
	 if(kopp_fc_debug_lvl == 2) DH2(recbuf[i+1]);					// <Print any received Bytes if Debuglevel = 2>

   }
   CC1100_DEASSERT;

	if(kopp_fc_debug_lvl == 2) DS_P(PSTR("\r\n"));					// <Print Linefeed if Debuglevel = 2>



  if (((memcmp(recbuf, lastrecblk, MAX_kopp_fc_NetMSG+1) != 0 ) && (recbuf[1]==7) && (recbuf[0] >= MAX_kopp_fc_NetMSG) && (recckserr!=1) && (kopp_fc_debug_lvl != 2)) || (kopp_fc_debug_lvl == 1))			
	{																// If (new Block not equal old block (without zeros at the end) and an Blocklength=7 
																	// and minimum one whole kopp message received and debuglevel <> 2) or debuglevel = 1 
																	// either Checksum is fine -> new command received; or print any block for debug purpose
																	// ------------------------------------------------------------------------------------------------------------------------------
	 DS_P(PSTR("kr"));												// Feed Back to FHEM: Was Kopp Receive + Received Data
     memcpy(lastrecblk, recbuf, MAX_kopp_fc_MSG+1);				// save new command (copy whole block incl. Fifo len information)
     for (uint8_t i=0; i<MAX_kopp_fc_NetMSG; i++)	 				// Read Receive String and ...
	 {																//
      DH2(recbuf[i+1]);											// print all received bytes
     }
	  if(kopp_fc_debug_lvl == 1) 									// 
	  {
       if (recckserr == 1)											// <Print Checksum Error information if Debuglevel = 1>
	   {
	   DS_P(PSTR(" Checksum Error, Calculated Cks: "));			// 
	   DH2(blkcks);
	   }
	  }

      DS_P(PSTR("\r\n"));											// 

    }

 } 
}

// ####RaspII  Hier gehts weiter. Synthesizer Calibration sollte im Empfangs Mode ab und an gemacht werden 
// noch messen wie schnell die _task hintereinander aufgerufen wird !!!!
 


// Kopp Free-Control Repetive task END
// ===========================================================================================================================================



#endif
