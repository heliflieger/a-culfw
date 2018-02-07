/* 
 * Copyright by O.Droegehorn / 
 *              DHS-Computertechnik GmbH
 * License: GPL v2
 */

#include <avr/interrupt.h>              // for cli, sei
#include <stdint.h>                     // for int8_t
#include <string.h>                     // for strlen
#ifdef USE_HAL
#include "hal.h"
#endif

#include <avr/pgmspace.h>               // for __LPM, PROGMEM
#include "board.h"                      // for HAS_ASKSIN, HAS_MORITZ, etc
#include <avr/io.h>                     // for _BV

#include "stringfunc.h"                 // for fromdec, fromhex

#ifdef HAS_INTERTECHNO

#include "cc1100.h"                     // for CC1100_CLEAR_OUT, etc
#include "delay.h"                      // for my_delay_us, my_delay_ms
#include "display.h"                    // for DC, DNL, DH2, DU
#include "fncollection.h"               // for EE_CC1100_CFG_SIZE, erb, etc
#include "intertechno.h"
#include "led.h"                        // for LED_OFF, LED_ON, SET_BIT
#include "rf_receive.h"                 // for set_txrestore, tx_report
#include "rf_mode.h"
#include "multi_CC.h"

#ifndef USE_RF_MODE
#ifdef HAS_ASKSIN
#include "rf_asksin.h"                  // for asksin_on, rf_asksin_init
#endif

#ifdef HAS_MORITZ
#include "rf_moritz.h"                  // for moritz_on, rf_moritz_init
#endif

static uint8_t intertechno_on = 0;
#endif

const PROGMEM const uint8_t CC1100_ITCFG[EE_CC1100_CFG_SIZE] = {
// CULFW   IDX NAME     RESET STUDIO COMMENT
   0x0D, // 00 IOCFG2   *29   *0B    GDO2 as serial output
   0x2E, // 01 IOCFG1    2E    2E    Tri-State
   0x2D, // 02 IOCFG0   *3F   *0C    GDO0 for input
   0x07, // 03 FIFOTHR   07   *47    
   0xD3, // 04 SYNC1     D3    D3    
   0x91, // 05 SYNC0     91    91    
   0x3D, // 06 PKTLEN   *FF    3D    
   0x04, // 07 PKTCTRL1  04    04    
   0x32, // 08 PKTCTRL0 *45    32    
   0x00, // 09 ADDR      00    00    
   0x00, // 0A CHANNR    00    00    
   0x06, // 0B FSCTRL1  *0F    06    152kHz IF Frquency
   0x00, // 0C FSCTRL0   00    00    
   0x10, // 0D FREQ2    *1E    21    433.92 (InterTechno Frequency)
   0xb0, // 0E FREQ1    *C4    65    
   0x71, // 0F FREQ0    *EC    e8    
   0x55, // 10 MDMCFG4  *8C    55    bWidth 325kHz
   0xe4, // 11 MDMCFG3  *22   *43    Drate:1500 ((256+228)*2^5)*26000000/2^28
   0x30, // 12 MDMCFG2  *02   *B0    Modulation: ASK
   0x23, // 13 MDMCFG1  *22    23    
   0xb9, // 14 MDMCFG0  *F8    b9    ChannelSpace: 350kHz
   0x00, // 15 DEVIATN  *47    00    
   0x07, // 16 MCSM2     07    07    
   0x00, // 17 MCSM1     30    30    
   0x18, // 18 MCSM0    *04    18    Calibration: RX/TX->IDLE
   0x14, // 19 FOCCFG   *36    14    
   0x6C, // 1A BSCFG     6C    6C    
   0x07, // 1B AGCCTRL2 *03   *03    42 dB instead of 33dB
   0x00, // 1C AGCCTRL1 *40   *40    
   0x90, // 1D AGCCTRL0 *91   *92    4dB decision boundery
   0x87, // 1E WOREVT1   87    87    
   0x6B, // 1F WOREVT0   6B    6B    
   0xF8, // 20 WORCTRL   F8    F8    
   0x56, // 21 FREND1    56    56    
   0x11, // 22 FREND0   *16    17    0x11 for no PA ramping
   0xE9, // 23 FSCAL3   *A9    E9    
   0x2A, // 24 FSCAL2   *0A    2A
   0x00, // 25 FSCAL1    20    00
   0x1F, // 26 FSCAL0    0D    1F    
   0x41, // 27 RCCTRL1   41    41    
   0x00, // 28 RCCTRL0   00    00    
};

uint16_t it_interval = 420;
uint8_t itv3_start_bit = 235;
uint16_t itv3_bit = 275;
uint16_t itv3_latch = 2650;
uint16_t itv3_low = 1180;
uint16_t itv3_sync = 10000;

#ifdef HAS_HOMEEASY
uint16_t heBit1 = 330;
uint16_t heBit0 = 1000;
uint16_t hesync = 5000;
uint16_t heeusync = 9000;
uint16_t heeusync_low = 1300;
#endif

#define DATATYPE_IT       1
#define DATATYPE_HE       2
#define DATATYPE_HEEU     3

uint8_t it_repetition = 6;
uint8_t restore_asksin = 0;
uint8_t restore_moritz = 0;
unsigned char it_frequency[] = {0x10, 0xb0, 0x71};

void
it_tunein(void)
{
		  int8_t i;
		  
#ifdef USE_HAL
		  hal_CC_GDO_init(CC_INSTANCE,INIT_MODE_OUT_CS_IN);
		  hal_enable_CC_GDOin_int(CC_INSTANCE,FALSE); // disable INT - we'll poll...
#else
		  EIMSK &= ~_BV(CC1100_INT);
  		SET_BIT( CC1100_CS_DDR, CC1100_CS_PIN ); // CS as output
#endif

  		CC1100_DEASSERT;                           // Toggle chip select signal
  		my_delay_us(30);
  		CC1100_ASSERT;
  		my_delay_us(30);
  		CC1100_DEASSERT;
  		my_delay_us(45);

		  ccStrobe( CC1100_SRES );                   // Send SRES command
  		my_delay_us(100);

		  CC1100_ASSERT;                             // load configuration
  		cc1100_sendbyte( 0 | CC1100_WRITE_BURST );
  		for(uint8_t i = 0; i < 13; i++) {
    		cc1100_sendbyte(__LPM(CC1100_ITCFG+i));
  		}																										// Tune to standard IT-Frequency
  		cc1100_sendbyte(it_frequency[0]);										// Modify Freq. for 433.92MHZ, or whatever
	  	cc1100_sendbyte(it_frequency[1]);
	 		cc1100_sendbyte(it_frequency[2]);  		
 			for (i = 16; i<EE_CC1100_CFG_SIZE; i++) {
   			cc1100_sendbyte(__LPM(CC1100_ITCFG+i));
 			}
  		CC1100_DEASSERT;

  		uint8_t *pa = EE_CC1100_PA;
		  CC1100_ASSERT;                             // setup PA table
  		cc1100_sendbyte( CC1100_PATABLE | CC1100_WRITE_BURST );
  		for (uint8_t i = 0;i<8;i++) {
    		cc1100_sendbyte(erb(pa++));
  		}
  		CC1100_DEASSERT;

  		ccStrobe( CC1100_SCAL );
  		my_delay_ms(1);
#ifndef USE_RF_MODE
  		cc_on = 1;																	// Set CC_ON
#endif
}

static void
send_IT_bit(uint8_t bit)
{
	if (bit == 1) {
  	CC1100_SET_OUT;         // High
  	my_delay_us(it_interval * 3);
 	  CC1100_CLEAR_OUT;       // Low
	  my_delay_us(it_interval);

  	CC1100_SET_OUT;         // High
  	my_delay_us(it_interval * 3);
 	  CC1100_CLEAR_OUT;       // Low
	  my_delay_us(it_interval);
  } else if (bit == 0) {
  	CC1100_SET_OUT;         // High
  	my_delay_us(it_interval);
 	  CC1100_CLEAR_OUT;       // Low
	  my_delay_us(it_interval * 3);

  	CC1100_SET_OUT;         // High
  	my_delay_us(it_interval);
 	  CC1100_CLEAR_OUT;       // Low
	  my_delay_us(it_interval * 3);
// Quad-State
  } else if (bit == 3) {
      CC1100_SET_OUT;         // High
      my_delay_us(it_interval * 3);
      CC1100_CLEAR_OUT;       // Low
      my_delay_us(it_interval);
      
      CC1100_SET_OUT;         // High
      my_delay_us(it_interval);
      CC1100_CLEAR_OUT;       // Low
      my_delay_us(it_interval * 3);
// Quad-State
  } else {
  	CC1100_SET_OUT;         // High
  	my_delay_us(it_interval);
 	  CC1100_CLEAR_OUT;       // Low
	  my_delay_us(it_interval * 3);

  	CC1100_SET_OUT;         // High
  	my_delay_us(it_interval * 3);
 	  CC1100_CLEAR_OUT;       // Low
	  my_delay_us(it_interval);  	
  }
}

static void
send_IT_latch_V3(void) {
 // int8_t k;
  CC1100_SET_OUT;         // High
	my_delay_us(itv3_bit);
  CC1100_CLEAR_OUT;       // Low
  //for(k = 0; k < 10; k++)  {
    my_delay_us(itv3_latch);
  //}
}

static void
send_IT_sync_V3(void) {
  //int8_t k;
  CC1100_SET_OUT;         // High
	my_delay_us(itv3_start_bit);
  CC1100_CLEAR_OUT;       // Low
  //for(k = 0; k < 40; k++)  {
    my_delay_us(itv3_sync);
  //}
}

#ifdef HAS_HOMEEASY
static void
send_IT_sync_HE(uint8_t mode)
{
    CC1100_SET_OUT;         // High
  	mode == DATATYPE_HE ? my_delay_us(heBit1) : my_delay_us(itv3_bit);
 	  CC1100_CLEAR_OUT;       // Low
	  mode == DATATYPE_HE ? my_delay_us(hesync) : my_delay_us(heeusync);
}

static void
send_IT_bit_HE(uint8_t bit, uint8_t mode)
{
	if (bit == 1) {
    CC1100_SET_OUT;         // High
  	mode == DATATYPE_HE ? my_delay_us(heBit1) : my_delay_us(itv3_bit);
 	  CC1100_CLEAR_OUT;       // Low
	  mode == DATATYPE_HE ? my_delay_us(heBit0) : my_delay_us(itv3_bit);
  } else {
    CC1100_SET_OUT;         // High
  	mode == DATATYPE_HE ? my_delay_us(heBit0) : my_delay_us(itv3_bit);
 	  CC1100_CLEAR_OUT;       // Low
	  mode == DATATYPE_HE ? my_delay_us(heBit1) : my_delay_us(heeusync_low);
  }
}
#endif

static void
send_IT_bit_V3(uint8_t bit)
{
	if (bit == 1) {
  	CC1100_SET_OUT;         // High
  	my_delay_us(itv3_bit);
 	  CC1100_CLEAR_OUT;       // Low
	  my_delay_us(itv3_low);

  	CC1100_SET_OUT;         // High
  	my_delay_us(itv3_bit);
 	  CC1100_CLEAR_OUT;       // Low
	  my_delay_us(itv3_bit);
  } else if (bit == 0) {
  	CC1100_SET_OUT;         // High
  	my_delay_us(itv3_bit);
 	  CC1100_CLEAR_OUT;       // Low
	  my_delay_us(itv3_bit);

  	CC1100_SET_OUT;         // High
  	my_delay_us(itv3_bit);
 	  CC1100_CLEAR_OUT;       // Low
	  my_delay_us(itv3_low);
  } else if (bit == 2) {
  	CC1100_SET_OUT;         // High
  	my_delay_us(itv3_bit);
 	  CC1100_CLEAR_OUT;       // Low
	  my_delay_us(itv3_low);

  	CC1100_SET_OUT;         // High
  	my_delay_us(itv3_bit);
 	  CC1100_CLEAR_OUT;       // Low
	  my_delay_us(itv3_low);  
  } else {
  	CC1100_SET_OUT;         // High
  	my_delay_us(itv3_bit);
 	  CC1100_CLEAR_OUT;       // Low
	  my_delay_us(itv3_bit);

  	CC1100_SET_OUT;         // High
  	my_delay_us(itv3_bit);
 	  CC1100_CLEAR_OUT;       // Low
	  my_delay_us(itv3_bit);  	
  }
}

static void
it_send (char *in, uint8_t datatype) {	

    //while (rf_isreceiving()) {
      //_delay_ms(1);
    //}
	  int8_t i, j;//, k;

		LED_ON();

    #if defined (HAS_IRRX) || defined (HAS_IRTX) //Blockout IR_Reception for the moment
      cli(); 
    #endif

#ifdef USE_RF_MODE
  change_RF_mode(RF_mode_intertechno);
#else
	// If NOT InterTechno mode
	if(!intertechno_on)  {
	#ifdef HAS_ASKSIN
		if (asksin_on) {
			restore_asksin = 1;
			asksin_on = 0;
			}
	#endif
	#ifdef HAS_MORITZ
		if(moritz_on) {
			restore_moritz = 1;
			moritz_on = 0;
		}
	#endif
		it_tunein();
		my_delay_ms(3);             // 3ms: Found by trial and error
    }
#endif
  	ccStrobe(CC1100_SIDLE);
  	ccStrobe(CC1100_SFRX );
  	ccStrobe(CC1100_SFTX );

	  ccTX();                       // Enable TX 
	
    int8_t sizeOfPackage = strlen(in)-1; // IT-V1 = 14, IT-V3 = 33, IT-V3-Dimm = 37
	  int8_t mode = 0; // IT V1
    //DU(sizeOfPackage, 3);
    if (sizeOfPackage == 33 || sizeOfPackage == 37) { 
      mode = 1; // IT V3
      
    }
    for(i = 0; i < it_repetition; i++)  {
      if (datatype == DATATYPE_IT) {
        if (mode == 1) {    
          send_IT_sync_V3();  
          send_IT_latch_V3();
        } else {
          // Sync-Bit for IT V1 send before package
          CC1100_SET_OUT;         // High
          my_delay_us(it_interval);
          CC1100_CLEAR_OUT;       // Low
          //for(k = 0; k < 40; k++)  {
          //  my_delay_us(it_interval);
          //}
          my_delay_us(itv3_sync);
        }
#ifdef HAS_HOMEEASY
      } else if (datatype == DATATYPE_HE) {
        send_IT_sync_HE(DATATYPE_HE);
      } else if (datatype == DATATYPE_HEEU) {
        send_IT_sync_HE(DATATYPE_HEEU);
#endif
      }
      uint8_t startCount = 1;
#ifdef HAS_HOMEEASY
      if (datatype == DATATYPE_HE || datatype == DATATYPE_HEEU) {
        startCount = 2;
      } 
#endif
      for(j = startCount; j < sizeOfPackage; j++)  {
	if(in[j+1] == '0') {
          if (datatype == DATATYPE_IT) {
            if (mode == 1) {
					    send_IT_bit_V3(0);
            } else {
					    send_IT_bit(0);
            }      
#ifdef HAS_HOMEEASY
          } else {
            send_IT_bit_HE(0, datatype);
#endif
          }
	} else if (in[j+1] == '1') {
          if (datatype == DATATYPE_IT) {
            if (mode == 1) {
					    send_IT_bit_V3(1);
            } else {
					    send_IT_bit(1);
            }
#ifdef HAS_HOMEEASY
          } else {
            send_IT_bit_HE(1, datatype);
#endif
          }
        } else if (in[j+1] == '2') {
          send_IT_bit_V3(2);
// Quad
        } else if (in[j+1] == 'D') {
          if (mode == 1) {
	        send_IT_bit_V3(3);
	  } else {
	    send_IT_bit(3);
	  }
// Quad
	} else {
      if (mode == 1) {
	    send_IT_bit_V3(3);
	  } else {
	    send_IT_bit(2);
	  }
	}
      }
      //if (mode == 1) {  
      //  send_IT_sync_V3();
      //}
		} //Do it n Times
#ifdef USE_RF_MODE
    if(!restore_RF_mode()) {
      // enable RX again
      if (TX_REPORT) {
        ccRX();
      } else {
        ccStrobe(CC1100_SIDLE);
      }
    }
#else
  	if(intertechno_on) {
			if(tx_report) {                               // Enable RX
	    	ccRX();
	  	} else {
		  	ccStrobe(CC1100_SIDLE);
			}
  	} 
  	#ifdef HAS_ASKSIN
   	  else if (restore_asksin) {
				restore_asksin = 0;
   			rf_asksin_init();
				asksin_on = 1;
   		 	ccRX();
  	}
  	#endif
	#ifdef HAS_MORITZ
      else if (restore_moritz) {
        restore_moritz = 0;
        rf_moritz_init();
    }
	#endif
      else {
        set_txrestore();
  	}	
#endif

    #if defined (HAS_IRRX) || defined (HAS_IRTX) //Activate IR_Reception again
      sei(); 
    #endif		  

		LED_OFF();
		MULTICC_PREFIX();
		DC('i');DC('s');
#ifdef HAS_HOMEEASY
    if (datatype == DATATYPE_HE) {
      DC('h');
    } else if (datatype == DATATYPE_HEEU) {
      DC('e');
    }
#endif
		for(j = 1; j < sizeOfPackage; j++)  {
		 	if(in[j+1] == '0') {
				DC('0');
			} else if (in[j+1] == '1') {
				DC('1');
			} else if (in[j+1] == '2') {
				DC('2');
                        } else if (in[j+1] == 'D') {
	  //if (mode == 1) {  
     		// Not supported
        //  } else {
	     DC('D');
          //}
			} else {
        if (datatype == DATATYPE_IT) {
          if (mode == 1) {  
     				DC('D');
          } else {
				    DC('F');
          }
        }
			}
		}
		DNL();
}


void
it_func(char *in)
{
	if (in[1] == 't') {
			fromdec (in+2, (uint8_t *)&it_interval);
			DU(it_interval,0); DNL();
	} else if (in[1] == 's') {
        if (in[2] == 'r') {		// Modify Repetition-counter
#ifdef ARM
            fromdec8(in+3, &it_repetition);
#else
            fromdec (in+3, (uint8_t *)&it_repetition);
#endif
            MULTICC_PREFIX();
            DU(it_repetition,0); DNL();
#ifdef HAS_HOMEEASY
            } else if (in[2] == 'h') {		// HomeEasy
                it_send (in, DATATYPE_HE);	
            } else if (in[2] == 'e') {		// HomeEasy EU
                it_send (in, DATATYPE_HEEU);	
#endif	
        } else {
            it_send (in, DATATYPE_IT);				// Sending real data
        } //sending real data
	} else if (in[1] == 'r') { // Start of "Set Frequency" (f)
#ifdef USE_RF_MODE
	  set_RF_mode(RF_mode_intertechno);
#else
		#ifdef HAS_ASKSIN
			if (asksin_on) {
				restore_asksin = 1;
				asksin_on = 0;
			}
		#endif
		#ifdef HAS_MORITZ
			if (moritz_on) {
				restore_moritz = 1;
				moritz_on = 0;
			}
		#endif
		it_tunein ();
		intertechno_on = 1;
#endif
	} else if (in[1] == 'f') { // Set Frequency
		  if (in[2] == '0' ) {
		  	it_frequency[0] = 0x10;
		  	it_frequency[1] = 0xb0;
		  	it_frequency[2] = 0x71;
		  } else {
				fromhex (in+2, it_frequency, 3);
			}
		  MULTICC_PREFIX();
			DC('i');DC('f');DC(':');
		  DH2(it_frequency[0]);
		  DH2(it_frequency[1]);
		  DH2(it_frequency[2]);
		  DNL();
	} else if (in[1] == 'x') { 		                    // Reset Frequency back to Eeprom value
#ifdef USE_RF_MODE
	  set_RF_mode(RF_mode_off);
#else
	  if(0) { ;
		#ifdef HAS_ASKSIN
		} else if (restore_asksin) {
			restore_asksin = 0;
			rf_asksin_init();
			asksin_on = 1;
			ccRX();
		#endif
		#ifdef HAS_MORITZ
		} else if (restore_moritz) {
			restore_moritz = 0;
			rf_moritz_init();
		#endif
		} else {
			ccInitChip(EE_CC1100_CFG);										// Set back to Eeprom Values
			if(tx_report) {                               // Enable RX
				ccRX();
			} else {
				ccStrobe(CC1100_SIDLE);
			}
		}
		intertechno_on = 0;
#endif
	} else if (in[1] == 'c') {		// Modify Clock-counter
        fromdec (in+1, (uint8_t *)&it_interval);
    }
}

#endif
