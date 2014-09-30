/*
 * Somfy RTS communication protocol
 *
 Copyright (c) 2014, Thomas Dankert <post@thomyd.de>
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:
 * Redistributions of source code must retain the above copyright
 notice, this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright
 notice, this list of conditions and the following disclaimer in the
 documentation and/or other materials provided with the distribution.
 * Neither the name of the <organization> nor the
 names of its contributors may be used to endorse or promote products
 derived from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdlib.h>
#include <util/parity.h>
#include <string.h>

#include "board.h"

#ifdef HAS_SOMFY_RTS

#include "delay.h"
#include "rf_send.h"
#include "rf_receive.h"
#include "led.h"
#include "cc1100.h"
#include "display.h"
#include "fncollection.h"
#include "somfy_rts.h"

#ifdef HAS_ASKSIN
#include "rf_asksin.h"
#endif

#ifdef HAS_MORITZ
#include "rf_moritz.h"
#endif

static uint8_t somfy_rts_on = 0;
typedef uint8_t somfy_rts_frame_t;

const PROGMEM const uint8_t CC1100_SOMFY_RTS_CFG[EE_CC1100_CFG_SIZE] = {
// CULFW   IDX NAME     RESET STUDIO COMMENT
	0x0D,// 00 IOCFG2   *29   *0B    GDO2 as serial output
	0x2E,// 01 IOCFG1    2E    2E    Tri-State
	0x2D,// 02 IOCFG0   *3F   *0C    GDO0 for input
	0x07,// 03 FIFOTHR   07   *47
	0xD3,// 04 SYNC1     D3    D3
	0x91,// 05 SYNC0     91    91
	0x3D,// 06 PKTLEN   *FF    3D
	0x04,// 07 PKTCTRL1  04    04
	0x32,// 08 PKTCTRL0 *45    32
	0x00,// 09 ADDR      00    00
	0x00,// 0A CHANNR    00    00
	0x06,// 0B FSCTRL1  *0F    06    152kHz IF Frquency
	0x00,// 0C FSCTRL0   00    00
	0x10,// 0D FREQ2    *1E    21    433.42 (10,AB,85: Somfy RTS Frequency)
	0xAB,// 0E FREQ1    *C4    65
	0xA5,// 0F FREQ0    *EC    e8
	0x55,// 10 MDMCFG4  *8C    55    bWidth 325kHz
	0x0A,// 11 MDMCFG3  *22   *43    Drate: 828 ((256+11)*2^5)*26000000/2^28
	0x30,// 12 MDMCFG2  *02   *B0    Modulation: ASK
	0x23,// 13 MDMCFG1  *22    23
	0xb9,// 14 MDMCFG0  *F8    b9    ChannelSpace: 350kHz
	0x00,// 15 DEVIATN  *47    00
	0x07,// 16 MCSM2     07    07
	0x00,// 17 MCSM1     30    30
	0x18,// 18 MCSM0    *04    18    Calibration: RX/TX->IDLE
	0x14,// 19 FOCCFG   *36    14
	0x6C,// 1A BSCFG     6C    6C
	0x07,// 1B AGCCTRL2 *03   *03    42 dB instead of 33dB
	0x00,// 1C AGCCTRL1 *40   *40
	0x90,// 1D AGCCTRL0 *91   *92    4dB decision boundery
	0x87,// 1E WOREVT1   87    87
	0x6B,// 1F WOREVT0   6B    6B
	0xF8,// 20 WORCTRL   F8    F8
	0x56,// 21 FREND1    56    56
	0x11,// 22 FREND0   *16    17    0x11 for no PA ramping
	0xE9,// 23 FSCAL3   *A9    E9
	0x2A,// 24 FSCAL2   *0A    2A
	0x00,// 25 FSCAL1    20    00
	0x1F,// 26 FSCAL0    0D    1F
	0x41,// 27 RCCTRL1   41    41
	0x00,// 28 RCCTRL0   00    00
};

uint8_t somfy_restore_asksin = 0;
uint8_t somfy_restore_moritz = 0;

uint8_t somfy_rts_repetition = 6;
uint16_t somfy_rts_interval = 1240; // symbol width in us -> ca. 828 Hz data rate
uint16_t somfy_rts_interval_half = 620;

static void somfy_rts_tunein(void) {
	EIMSK &= ~_BV(CC1100_INT);
	SET_BIT(CC1100_CS_DDR, CC1100_CS_PIN); // CS as output

	CC1100_DEASSERT;// Toggle chip select signal
	my_delay_us(30);
	CC1100_ASSERT;
	my_delay_us(30);
	CC1100_DEASSERT;
	my_delay_us(45);

	ccStrobe( CC1100_SRES);// Send SRES command
	my_delay_us(100);

	// load configuration
	CC1100_ASSERT;

	cc1100_sendbyte( 0 | CC1100_WRITE_BURST );
	for(uint8_t i = 0; i < EE_CC1100_CFG_SIZE; i++) {
		cc1100_sendbyte(__LPM(CC1100_SOMFY_RTS_CFG+i));
	}

	CC1100_DEASSERT;

	// setup PA table
	uint8_t *pa = EE_CC1100_PA;
	CC1100_ASSERT;
	cc1100_sendbyte( CC1100_PATABLE | CC1100_WRITE_BURST);
	for (uint8_t i = 0; i < 8; i++) {
		cc1100_sendbyte(erb(pa++));
	}
	CC1100_DEASSERT;

	// Set CC_ON
	ccStrobe( CC1100_SCAL);
	my_delay_ms(1);
	cc_on = 1;
}

static void send_somfy_rts_bitZero(void) {
	// Somfy RTS bits are manchester encoded: 0 = high->low
	CC1100_OUT_PORT |= _BV(CC1100_OUT_PIN);// High
	my_delay_us(somfy_rts_interval_half);
	CC1100_OUT_PORT &= ~_BV(CC1100_OUT_PIN);// Low
	my_delay_us(somfy_rts_interval_half);
}

static void send_somfy_rts_bitOne(void) {
	// Somfy RTS bits are manchester encoded: 1 = low->high
	CC1100_OUT_PORT &= ~_BV(CC1100_OUT_PIN);// Low
	my_delay_us(somfy_rts_interval_half);
	CC1100_OUT_PORT |= _BV(CC1100_OUT_PIN);// High
	my_delay_us(somfy_rts_interval_half);
}

static void send_somfy_rts_frame(somfy_rts_frame_t *frame, int8_t hwPulses) {
	// send hardware sync (pulses of ca. double length)
	for (int8_t i = 0; i < hwPulses; i++) {
		CC1100_OUT_PORT |= _BV(CC1100_OUT_PIN);		// High
		my_delay_us(2550);
		CC1100_OUT_PORT &= ~_BV(CC1100_OUT_PIN);// Low
		my_delay_us(2550);
	}

	// send software sync (4 x symbol width high, half symbol width low)
	CC1100_OUT_PORT |= _BV(CC1100_OUT_PIN);// High
	my_delay_us(4860);
	CC1100_OUT_PORT &= ~_BV(CC1100_OUT_PIN);// Low
	my_delay_us(somfy_rts_interval_half);

	// Send the user data
	for (int8_t i = 0; i < SOMFY_RTS_FRAME_SIZE; i++) {
		uint16_t mask = 0x80; // mask to send bits (MSB first)
		uint8_t d = frame[i];
		for (int8_t j = 0; j < 8; j++) {
			if ((d & mask) == 0) {
				send_somfy_rts_bitZero();
			} else {
				send_somfy_rts_bitOne();
			}
			mask >>= 1; //get next bit
		} //end of byte
	} //end of data

	// send inter-frame gap
	// if last bit = 0, silence is 1/2 symbol longer
	CC1100_OUT_PORT &= ~_BV(CC1100_OUT_PIN);// Low
	my_delay_us(30415 + ((frame[6] >> 7) & 1) ? 0 : somfy_rts_interval_half);
}

static uint8_t somfy_rts_calc_checksum(somfy_rts_frame_t *frame) {
	uint8_t checksum = 0;
	for (int8_t i = 0; i < 7; i++) {
		checksum = checksum ^ frame[i] ^ (frame[i] >> 4);
	}

	return checksum;
}

static void somfy_rts_send(char *in) {
	int8_t i, j;

	// input is in format: Ys_key_ctrl_cks_rollcode_a0_a1_a2
	// Ys ad 20 0ae3 a2 98 42

	// create somfy frame from the given input
	// 0   |    1     |   2     3    | 4   5   6
	// key | ctrl+cks | rolling_code | address

	uint8_t buf = 0;
	somfy_rts_frame_t *airdata = malloc(SOMFY_RTS_FRAME_SIZE);

	fromhex(in+2, &buf, 1);// key
	airdata[0] = buf;

	fromhex(in+4, &buf, 1);// ctrl+cks (cks set to 0)
	airdata[1] = buf & 0xF0;

	fromhex(in+6, &buf, 1);// rolling code 1/2
	airdata[2] = buf;

	fromhex(in+8, &buf, 1);// rolling code 2/2
	airdata[3] = buf;

	fromhex(in+10, &buf, 1);// address a0
	airdata[6] = buf;

	fromhex(in+12, &buf, 1);// address a1
	airdata[5] = buf;

	fromhex(in+14, &buf, 1);// address a2
	airdata[4] = buf;

	// calculate checksum
	airdata[1] |= (somfy_rts_calc_checksum(airdata) & 0x0F);
	
	// save unencrypted data to return later
	somfy_rts_frame_t *unencrypted = malloc(SOMFY_RTS_FRAME_SIZE);
	memcpy(unencrypted, airdata, SOMFY_RTS_FRAME_SIZE);

	// "encrypt"
	for (i = 1; i < SOMFY_RTS_FRAME_SIZE; i++) {
		airdata[i] = airdata[i] ^ airdata[i-1];
	}

	LED_ON();

#if defined (HAS_IRRX) || defined (HAS_IRTX) // Blockout IR_Reception for the moment
	cli();
#endif

	// If NOT in Somfy mode
	if (!somfy_rts_on) {
	#ifdef HAS_ASKSIN
		if (asksin_on) {
			somfy_restore_asksin = 1;
			asksin_on = 0;
		}
	#endif
	#ifdef HAS_MORITZ
		if(moritz_on) {
			somfy_restore_moritz = 1;
			moritz_on = 0;
		}
	#endif
		somfy_rts_tunein();
		my_delay_ms(3);             // 3ms: Found by trial and error
	}
	ccStrobe(CC1100_SIDLE);
	ccStrobe(CC1100_SFRX);
	ccStrobe(CC1100_SFTX);

	// enable TX
	ccTX();

	// send wakeup pulse
	CC1100_OUT_PORT |= _BV(CC1100_OUT_PIN);// High
	my_delay_us(9415);
	CC1100_OUT_PORT &= ~_BV(CC1100_OUT_PIN);// Low
	my_delay_ms(89);// should be 89565 us
	my_delay_us(565);

	// send the whole frame several times
	for(i = 0; i < somfy_rts_repetition; i++) {
		send_somfy_rts_frame(airdata, (i == 0) ? 2 : 7); // send 2 hw Sync pulses at first, and 7 for repeated frames
	}

	if (somfy_rts_on) {
		// enable RX again
		if (tx_report) {
			ccRX();
		} else {
			ccStrobe(CC1100_SIDLE);
		}
	}
 	#ifdef HAS_ASKSIN
   	else if (somfy_restore_asksin) {
		somfy_restore_asksin = 0;
   		rf_asksin_init();
		asksin_on = 1;
   	 	ccRX();
  	}
  	#endif
	#ifdef HAS_MORITZ
	else if (somfy_restore_moritz) {
		somfy_restore_moritz = 0;
		rf_moritz_init();
	}
	#endif
  	else {
    	set_txrestore();
  	}

#if defined (HAS_IRRX) || defined (HAS_IRTX) //Activate IR_Reception again
	sei();
#endif

	LED_OFF();

	DC('Y');
	DC('s');
	for(j = 0; j < SOMFY_RTS_FRAME_SIZE; j++) {
		display_hex2(unencrypted[j]);
	}
	DNL();
	
	free(unencrypted);
	free(airdata);
}

void somfy_rts_func(char *in) {
	if (in[1] == 's') {
		somfy_rts_send(in); // Send real data

	} else if (in[1] == 'r') { // Set repetition
		fromdec (in+2, (uint8_t *)&somfy_rts_repetition);
		DC('Y');DC('r');DC(':');
		DU(somfy_rts_repetition, 0);
		DNL();

	} else if (in[1] == 't') { // Set symbol width
		if (in[2] == '0' ) {
			somfy_rts_interval = 1240;
		} else {
			fromdec (in+2, (uint8_t *)&somfy_rts_interval);
		}
		somfy_rts_interval_half = somfy_rts_interval / 2;

		DC('Y');DC('t');DC(':');
		DU(somfy_rts_interval, 6);
		DNL();
	} else if (in[1] == 'x') { 	// Reset Frequency back to Eeprom value
		if(0) { ;
		#ifdef HAS_ASKSIN
		} else if (somfy_restore_asksin) {
			somfy_restore_asksin = 0;
			rf_asksin_init();
			asksin_on = 1;
			ccRX();
		#endif
		#ifdef HAS_MORITZ
		} else if (somfy_restore_moritz) {
			somfy_restore_moritz = 0;
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
		somfy_rts_on = 0;
	}
}

#endif
