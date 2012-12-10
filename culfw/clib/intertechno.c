/* 
 * Copyright by O.Droegehorn / 
 *              DHS-Computertechnik GmbH
 * License: GPL v2
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <util/parity.h>
#include <string.h>

#include "board.h"

#ifdef HAS_INTERTECHNO

#include "delay.h"
#include "rf_send.h"
#include "rf_receive.h"
#include "led.h"
#include "cc1100.h"
#include "display.h"
#include "fncollection.h"
#include "fht.h"
#include "intertechno.h"

#ifdef HAS_ASKSIN
#include "rf_asksin.h"
#endif

#ifdef HAS_MORITZ
#include "rf_moritz.h"
#endif

static uint8_t intertechno_on = 0;

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
uint16_t it_repetition = 6;
uint8_t restore_asksin = 0;
uint8_t restore_moritz = 0;
unsigned char it_frequency[] = {0x10, 0xb0, 0x71};

static void
it_tunein(void)
{
		  int8_t i;
		  
 		 	EIMSK &= ~_BV(CC1100_INT);                 
  		SET_BIT( CC1100_CS_DDR, CC1100_CS_PIN ); // CS as output

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
  		cc_on = 1;																	// Set CC_ON	
}

static void
send_IT_bit(uint8_t bit)
{
	if (bit == 1) {
  	CC1100_OUT_PORT |= _BV(CC1100_OUT_PIN);         // High
  	my_delay_us(it_interval * 3);
 	  CC1100_OUT_PORT &= ~_BV(CC1100_OUT_PIN);       // Low
	  my_delay_us(it_interval);

  	CC1100_OUT_PORT |= _BV(CC1100_OUT_PIN);         // High
  	my_delay_us(it_interval * 3);
 	  CC1100_OUT_PORT &= ~_BV(CC1100_OUT_PIN);       // Low
	  my_delay_us(it_interval);
  } else if (bit == 0) {
  	CC1100_OUT_PORT |= _BV(CC1100_OUT_PIN);         // High
  	my_delay_us(it_interval);
 	  CC1100_OUT_PORT &= ~_BV(CC1100_OUT_PIN);       // Low
	  my_delay_us(it_interval * 3);

  	CC1100_OUT_PORT |= _BV(CC1100_OUT_PIN);         // High
  	my_delay_us(it_interval);
 	  CC1100_OUT_PORT &= ~_BV(CC1100_OUT_PIN);       // Low
	  my_delay_us(it_interval * 3);
  } else {
  	CC1100_OUT_PORT |= _BV(CC1100_OUT_PIN);         // High
  	my_delay_us(it_interval);
 	  CC1100_OUT_PORT &= ~_BV(CC1100_OUT_PIN);       // Low
	  my_delay_us(it_interval * 3);

  	CC1100_OUT_PORT |= _BV(CC1100_OUT_PIN);         // High
  	my_delay_us(it_interval * 3);
 	  CC1100_OUT_PORT &= ~_BV(CC1100_OUT_PIN);       // Low
	  my_delay_us(it_interval);  	
  }
}

static void
it_send (char *in) {	
	  int8_t i, j, k;

		LED_ON();

    #if defined (HAS_IRRX) || defined (HAS_IRTX) //Blockout IR_Reception for the moment
      cli(); 
    #endif
			

  	// If NOT InterTechno mode && NOT 433 MHZ Device
  	#ifdef MULTI_FREQ_DEVICE
 	 		if((!intertechno_on) && (bit_is_set(MARK433_PIN, MARK433_BIT)))  {
 	 	#else
 	 		if(!intertechno_on)  {
 	 	#endif
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
  	ccStrobe(CC1100_SIDLE);
  	ccStrobe(CC1100_SFRX );
  	ccStrobe(CC1100_SFTX );

	  ccTX();                       // Enable TX 
	
		for(i = 0; i < it_repetition; i++)  {
		  for(j = 1; j < 13; j++)  {
			  if(in[j+1] == '0') {
					send_IT_bit(0);
				} else if (in[j+1] == '1') {
					send_IT_bit(1);
				} else {
					send_IT_bit(2);
				}
			}
			// Sync-Bit
		  CC1100_OUT_PORT |= _BV(CC1100_OUT_PIN);         // High
		  my_delay_us(it_interval);
		  CC1100_OUT_PORT &= ~_BV(CC1100_OUT_PIN);       // Low
		  for(k = 0; k < 31; k++)  {
			  my_delay_us(it_interval);
			}
		} //Do it n Times
	
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
		moritz_on = 1;
		ccRX();
	}
	#endif
  	else {
    	set_txrestore();
  	}	

    #if defined (HAS_IRRX) || defined (HAS_IRTX) //Activate IR_Reception again
      sei(); 
    #endif		  

		LED_OFF();
	
		DC('i');DC('s');
		for(j = 1; j < 13; j++)  {
		 	if(in[j+1] == '0') {
				DC('0');
			} else if (in[j+1] == '1') {
				DC('1');
			} else {
				DC('F');
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
				fromdec (in+3, (uint8_t *)&it_repetition);
				DU(it_repetition,0); DNL();
			} else {
				it_send (in);				// Sending real data
		} //sending real data
	} else if (in[1] == 'r') { // Start of "Set Frequency" (f)
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
	} else if (in[1] == 'f') { // Set Frequency
		  if (in[2] == '0' ) {
		  	it_frequency[0] = 0x10;
		  	it_frequency[1] = 0xb0;
		  	it_frequency[2] = 0x71;
		  } else {
				fromhex (in+2, it_frequency, 3);
			}
			DC('i');DC('f');DC(':');
		  DH2(it_frequency[0]);
		  DH2(it_frequency[1]);
		  DH2(it_frequency[2]);
		  DNL();
	} else if (in[1] == 'x') { 		                    // Reset Frequency back to Eeprom value
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
			moritz_on = 1;
			ccRX();
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
	}
}

#endif
