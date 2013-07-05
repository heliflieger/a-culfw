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
#include "ir.h"
#include "irmp.h"

#define RGB_ON 		0x87
#define RGB_OFF		0x86
#define RGB_DIMUP	0x85
#define RGB_DIMDOWN	0x84

#define RGB_RED		0x89
#define RGB_GREEN	0x88
#define RGB_BLUE	0x8A
#define RGB_LIGHTBLUE	0x94
#define RGB_WHITE	0x8B
#define RGB_YELLOW	0x91
#define RGB_PURPLE	0x92
#define RGB_LIGHTPURPLE	0x96
#define RGB_ORANGE	0x95
#define RGB_SMOOTH	0x93
#define RGB_FLASH	0x8f
#define RGB_FADE	0x9b
#define RGB_STROBE	0x97

const uint8_t levels[] = {
    RGB_BLUE, RGB_RED, RGB_GREEN, RGB_LIGHTBLUE, RGB_YELLOW, RGB_PURPLE, RGB_LIGHTPURPLE, RGB_ORANGE, 0x98, 0x8e, RGB_WHITE, RGB_SMOOTH, RGB_FLASH, RGB_FADE, RGB_STROBE, RGB_ON
};

static uint8_t level = 0;
static uint8_t is_on = 0;

#define EE_CH1_ADDR      (uint8_t *)700
#define EE_CH2_ADDR      (uint8_t *)703

static char mycmd[65];
static uint8_t mypos = 0;
static uint8_t learning = 0;
static uint32_t learn_timer = 0;
static uint8_t ADDR[2][3];


void read_addr(void) {
  ADDR[0][0] = erb( EE_CH1_ADDR   );
  ADDR[0][1] = erb( EE_CH1_ADDR+1 );
  ADDR[0][2] = erb( EE_CH1_ADDR+2 );
  ADDR[1][0] = erb( EE_CH2_ADDR   );
  ADDR[1][1] = erb( EE_CH2_ADDR+1 );
  ADDR[1][2] = erb( EE_CH2_ADDR+2 );
}

static uint8_t CHAN;

void action( uint8_t ch, uint8_t code ) {
  IRMP_DATA irmp_data;

  irmp_data.protocol = 2;
  irmp_data.address  = 0xFE01;
  irmp_data.command  = 0;
  irmp_data.flags    = 0;

  CHAN = ch;

  LED_ON();

  switch (code) {
    case 0:
      irmp_data.command = RGB_OFF;
      is_on = 0;
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
      level = code-1;
      irmp_data.command = levels[level];
      break;
    case 16:
    case 17:
      if (is_on) {
        level = (level+1) & 15;
        irmp_data.command = levels[level];
      } else {
        irmp_data.command = RGB_ON;
      }
      is_on = 1;
      break;
    case 18: // toggle
      if (is_on) {
        irmp_data.command = RGB_OFF;
        is_on = 0;
      } else {
        irmp_data.command = RGB_ON;
        is_on = 1;
      }
      break;
    case 19: // dim up
      irmp_data.command = RGB_DIMUP;
      break;
    case 20: // dim down
      irmp_data.command = RGB_DIMDOWN;
      break;
    case 21: // dim up / down
      break;
  }

  if (irmp_data.command) {   
    irsnd_send_data (&irmp_data, TRUE);
  }
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

   	read_addr();
        learning = 0;
        xled_pattern=0x0000;

      } else {

	for (uint8_t i=0;i<2;i++) {
          if ( (ad[0] == ADDR[i][0]) && (ad[1] == ADDR[i][1]) && (ad[2] == ADDR[i][2]) )
            action( i, ad[3]&0x1f );
        } 

      }
    }

    // reset
    mypos = 0;
    return;
  }

  mycmd[mypos] = data;
  mypos = (mypos+1) & 63;
}

void pin_set( uint8_t value ) {
  if (value) {
    PORTD |= (0x80>>CHAN);
  } else {
    PORTD &= ~(0x80>>CHAN);
  }
}

int main(void) {
  int16_t d;
  uint8_t loop = 0;

  wdt_disable();

  // no internal PWM
  DDRD  |= _BV( 7 ) | _BV( 6 );

  KEY_PORT |= _BV( KEY_PIN ); // pull
 
  irsnd_set_callback_ptr(pin_set);
  culfw_init();

  // enable reception
  tx_report = 0x21;
  set_txrestore();

  xled_pattern=0x0000;

  wdt_enable(WDTO_2S);

  is_on = 1;
 
  read_addr();

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

    if (loop++<70)
      continue;

    loop = 0;

  }

}
