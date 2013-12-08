/* Copyright Rudolf Koenig, 2008.
   Released under the GPL Licence, Version 2
   Inpired by the MyUSB USBtoSerial demo, Copyright (C) Dean Camera, 2008.
*/

#include <avr/boot.h>
#include <avr/power.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>

#include <string.h>

#include "board.h"
#include "private_channel.h"
#include "rf_receive.h"
#include "stringfunc.h"
#include "fncollection.h"
#include "led.h"
#include "clock.h"
#include "../culfw.h"

const uint8_t levels[] = {
    1, 2, 3, 4, 6, 8, 11, 16, 23, 32, 45, 64, 91, 128, 181, 255
};

#define EE_CH1_ADDR      (uint8_t *)700
#define EE_CH1_VALUE     (EE_CH1_ADDR+3)
#define EE_CH1_STATE     (EE_CH1_VALUE+1)

#define EE_CH2_ADDR      (EE_CH1_STATE+1)
#define EE_CH2_VALUE     (EE_CH2_ADDR+3)
#define EE_CH2_STATE     (EE_CH2_VALUE+1)

#define BIT_OFF          1
#define BIT_DOWN         2

static char mycmd[256];
static uint8_t mypos = 0;
static uint8_t learning = 0;
static uint32_t learn_timer = 0;
static uint32_t last_dud[2] = {0,0};

static uint8_t state[2][3];

void update( uint8_t ch ) {

   uint8_t bm = ch ? (_BV(COM2B0) | _BV(COM2B1)) : (_BV(COM2A0) | _BV(COM2A1)); 

   if (bit_is_set(state[ch][1], BIT_OFF)) {
     TCCR2A &= ~bm;
     state[ch][2] = 0;
   } else {
     state[ch][2] = levels[state[ch][0]];
     TCCR2A |= bm;
   }

}

void action( uint8_t ch, uint8_t code ) {

  switch (code) {
    case 0:
      state[ch][1] |= _BV( BIT_OFF ); // set OFF bit
      break;
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
    case 16:
      state[ch][0] = code-1; // dim value direct
      state[ch][1] &= ~_BV( BIT_OFF ); // clear OFF bit
      break;
    case 17:
      state[ch][1] &= ~_BV( BIT_OFF ); // clear OFF bit
      break;
    case 18:
      state[ch][1] ^= _BV( BIT_OFF ); // toggle OFF bit
      break;
    case 19: // dim up

      if (bit_is_set(state[ch][1], BIT_OFF)) {
        state[ch][0] = 0;
      } else if (state[ch][0]<15)
        state[ch][0]++;

      state[ch][1] &= ~_BV( BIT_OFF ); // clear OFF bit
      break;
    case 20: // dim down
      if (!bit_is_set(state[ch][1], BIT_OFF))
        if (state[ch][0]>0)
          state[ch][0]--;
      break;
    case 21: // dim up / down

      if (bit_is_set(state[ch][1], BIT_OFF)) {
        state[ch][1] &= ~_BV( BIT_DOWN ); // clear DOWN bit
      } else if(last_dud[ch]+100<ticks)
        state[ch][1] ^= _BV( BIT_DOWN ); // toggle DOWN bit

      if (bit_is_set(state[ch][1], BIT_DOWN)) {
        if (!bit_is_set(state[ch][1], BIT_OFF))
          if (state[ch][0]>0) {
            state[ch][0]--;
          }
      } else {
 
        if (bit_is_set(state[ch][1], BIT_OFF)) {
          state[ch][0] = 0;
        } else if (state[ch][0]<15) {
          state[ch][0]++;
        }
        state[ch][1] &= ~_BV( BIT_OFF ); // clear OFF bit
      }
      last_dud[ch] = ticks;
      break;
    }

    state[ch][0] &= 0xf;
    update( ch );

#ifdef REL_PORT
    if (bit_is_set(state[ch][1], BIT_OFF)) {
      REL_PORT &= ~_BV( REL_PIN );
    } else {
      REL_PORT |= _BV( REL_PIN );
    }
#endif

}

// messages received from air
void private_putchar(char data) {
  uint8_t ad[4];

  if (data == 13)
    return;
  if (data == 10) {
    // EOL
    mycmd[mypos] = 0;

    // process
    if (mycmd[0] == 'F' && strlen(mycmd)>6) {
      fromhex(mycmd+1, ad, 1);
      fromhex(mycmd+3, ad+1, 1);
      fromhex(mycmd+5, ad+2, 1);
      fromhex(mycmd+7, ad+3, 1);

      if (learning) {

        if (learning==1) {
          ewb(EE_CH1_ADDR, ad[0]);
          ewb(EE_CH1_ADDR+1, ad[1]);
          ewb(EE_CH1_ADDR+2, ad[2]);
        } else if (learning==2) {
          ewb(EE_CH2_ADDR, ad[0]);
          ewb(EE_CH2_ADDR+1, ad[1]);
          ewb(EE_CH2_ADDR+2, ad[2]);
        }

        learning = 0;
        xled_pattern=0x0000;

      } else {

        // for us?
        if ( (ad[0] == erb(EE_CH1_ADDR)) && (ad[1] == erb(EE_CH1_ADDR+1)) && (ad[2] == erb(EE_CH1_ADDR+2)) ) {
          action( 0, ad[3]&0x1f );
        } 
        if ( (ad[0] == erb(EE_CH2_ADDR)) && (ad[1] == erb(EE_CH2_ADDR+1)) && (ad[2] == erb(EE_CH2_ADDR+2)) ) {
          action( 1, ad[3]&0x1f );
        }

      }
    }

    // reset
    mypos = 0;
    return;
  }

  mycmd[mypos] = data;
  mypos++;
}


int main(void) {
  int16_t d;
  uint8_t loop = 100;

  wdt_disable();

#ifndef ADB2001
  // normal PWM
  DDRD  |= _BV( 7 ) | _BV( 6 );
  PORTD |= _BV( 7 ) | _BV( 6 );

  OCR2A = 1;
  OCR2B = 1;
//  TCCR2A = _BV( WGM20) | _BV( WGM21) | _BV( COM2A1 ) | _BV( COM2A0 ) | _BV( COM2B1 ) | _BV( COM2B0 );
  TCCR2A = _BV( WGM20) | _BV( WGM21);
  TCCR2B = _BV( CS21 ) | _BV( CS21 ) | _BV( CS22 );

  KEY_PORT |= _BV( KEY_PIN ); // pull
#endif
 
  culfw_init();

  state[0][0] = erb( EE_CH1_VALUE ) & 0xf;
  state[0][1] = 0; // erb( EE_CH1_STATE );
  update( 0 );

  state[1][0] = erb( EE_CH2_VALUE ) & 0xf;
  state[1][1] = 0; // erb( EE_CH2_STATE );
  update( 1 );

#ifdef REL_PORT
  REL_DDR |= _BV( REL_PIN );
  REL_PORT &= ~_BV( REL_PIN );
#endif

  // enable reception
  tx_report = 0x21;
  set_txrestore();

  xled_pattern=0x0000;

  wdt_enable(WDTO_2S);

  for(;;) {
    culfw_loop();

    if(bit_is_set( KEY_IN, KEY_PIN )) {
      learn_timer = ticks;
    }

    if(learn_timer+500<ticks) {
      learning++;
      if (learning>2)
        learning = 0;
      switch(learning) {
        case 1:
          xled_pattern=0x0001;
          break;
        case 2:
          xled_pattern=0x000a;
          break;
        default:
          xled_pattern=0x0000;
          break;
      }
      learn_timer = ticks;
    }

    // store values every 30 sek.
    if ((ticks%7500)==0) {
      if (state[0][0] != erb( EE_CH1_VALUE ))
        ewb(EE_CH1_VALUE, state[0][0]);
      if (state[1][0] != erb( EE_CH2_VALUE ))
        ewb(EE_CH2_VALUE, state[1][0]);
    }

    if (loop++<70)
      continue;

    loop = 0;

    d = OCR2A - state[0][2];
    if (d<0) {
      OCR2A++;
    } else if (d>0) {
      OCR2A--;
    }

    d = OCR2B - state[1][2];
    if (d<0) {
      OCR2B++;
    } else if (d>0) {
      OCR2B--;
    }



  }

}
