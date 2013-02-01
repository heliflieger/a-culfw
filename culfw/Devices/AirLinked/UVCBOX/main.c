#include <avr/boot.h>
#include <avr/power.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>

#include <string.h>

#include "main.h"

volatile uint32_t ticks;
uint32_t led_to, rela_to, relb_to;

#define DELAY 180 // seconds

ISR(TIMER0_COMPA_vect, ISR_BLOCK) {
  // 125Hz
  ticks++;
}

int main(void) {
  wdt_disable();

  // Setup the timers. Are needed for watchdog-reset
  OCR0A  = 249;                            // Timer0: 0.008s = 8MHz/256/250
  TCCR0B = _BV(CS02);
  TCCR0A = _BV(WGM01);
  TIMSK0 = _BV(OCIE0A);

  ticks   = 0;
  led_to  = 0;
  rela_to = 0;
  relb_to = 0;

  LED_DDR |= _BV( LED_PIN );
  RELA_DDR |= _BV( RELA_PIN );
  RELB_DDR |= _BV( RELB_PIN );

  KEY_DDR &= ~_BV( KEY_PIN );
  INPA_DDR &= ~_BV( INPA_PIN );
  INPB_DDR &= ~_BV( INPB_PIN );

  sei();

  while (1) {

   if (!bit_is_set(KEY_IN, KEY_PIN)) {
     LED_PORT &= ~_BV( LED_PIN );
     led_to = ticks + (DELAY*125);
   }

   if (!bit_is_set(INPA_IN, INPA_PIN)) {
     RELA_PORT |= _BV( RELA_PIN );
     rela_to = ticks + (DELAY*125);
   }

   if (!bit_is_set(INPB_IN, INPB_PIN)) {
     RELB_PORT |= _BV( RELB_PIN );
     relb_to = ticks + (DELAY*125);
   }

   // check timeouts
   if (led_to<ticks)
     LED_PORT |= _BV( LED_PIN );

   if (rela_to<ticks)
     RELA_PORT &= ~_BV( RELA_PIN );

   if (relb_to<ticks)
     RELB_PORT &= ~_BV( RELB_PIN );

   _delay_ms( 10 );
  }
}
