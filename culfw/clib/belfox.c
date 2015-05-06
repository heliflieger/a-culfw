/* 
   Copyright by GWu
   License: LGPL v2.1
   --
   provides send function for BelFox door sender
   see http://forum.fhem.de/index.php/topic,36810.0.html for discussion and documentation
   --
*/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <util/parity.h>
#include <string.h>

#include "board.h"

#ifdef HAS_BELFOX

#include "delay.h"
#include "belfox.h"
#include "rf_receive.h"
#include "led.h"
#include "cc1100.h"

#ifdef HAS_MORITZ
#include "rf_moritz.h"
#endif

// define timings
#define BELFOX_ZERO_HIGH               1000  // 1000uS
#define BELFOX_ZERO_LOW  2*BELFOX_ZERO_HIGH  
#define BELFOX_ONE_HIGH  2*BELFOX_ZERO_HIGH
#define BELFOX_ONE_LOW     BELFOX_ZERO_HIGH
#define BELFOX_PAUSE                     35  // 35ms
#define BELFOX_REPEAT                     4  // repeat 4 times
#define BELFOX_LEN                       12  // length of message

// send sync flag
static void
send_sync(void)
{
  CC1100_OUT_PORT |= _BV(CC1100_OUT_PIN);         // High
  my_delay_us(BELFOX_ZERO_HIGH);
}
 
// send belfox bit (low/high)
static void
send_bit(uint8_t bit)
{
  CC1100_OUT_PORT &= ~_BV(CC1100_OUT_PIN);        // Low
  my_delay_us(bit ? BELFOX_ONE_LOW  : BELFOX_ZERO_LOW );

  CC1100_OUT_PORT |= _BV(CC1100_OUT_PIN);         // High
  my_delay_us(bit ? BELFOX_ONE_HIGH : BELFOX_ZERO_HIGH );
}

void
send_belfox(char *msg)
{

  uint8_t repeat=BELFOX_REPEAT;
  uint8_t len=strnlen(msg,BELFOX_LEN+2)-1;
  
  // assert if length incorrect
  if (len != BELFOX_LEN) return;
  
  LED_ON();

#if defined (HAS_IRRX) || defined (HAS_IRTX) // Block IR_Reception
  cli();
#endif

#ifdef HAS_MORITZ
  uint8_t restore_moritz = 0;
  if(moritz_on) {
    restore_moritz = 1;
    moritz_on = 0;
    set_txreport("21");
  }
#endif

  if(!cc_on)
    set_ccon();
  ccTX();                                       // Enable TX 
  do {
    send_sync();                                // sync

    for(int i = 1; i <= BELFOX_LEN; i++)        // loop input, for example 'L111001100110'
      send_bit(msg[i] == '1');

    CC1100_OUT_PORT &= ~_BV(CC1100_OUT_PIN);    // final low to complete last bit
        
    my_delay_ms(BELFOX_PAUSE);                  // pause

  } while(--repeat > 0);

  if(tx_report) {                               // Enable RX
    ccRX();
  } else {
    ccStrobe(CC1100_SIDLE);
  }

#if defined (HAS_IRRX) || defined (HAS_IRTX) // Activate IR_Reception
  sei(); 
#endif

#ifdef HAS_MORITZ
  if(restore_moritz)
    rf_moritz_init();
#endif

  LED_OFF();
}

#endif
