#include <avr/boot.h>
#include <avr/power.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>

#include <string.h>

#include "../board.h"

const uint8_t levels[] = {
    0, 1, 2, 2, 2, 3, 3, 4, 5, 6, 7, 8, 10, 11, 13, 16, 19, 23,
    27, 32, 38, 45, 54, 64, 76, 91, 108, 128, 152, 181, 215, 255
};

int main(void) {
  wdt_disable();

  DDRD  |= _BV( 7 ) | _BV( 6 );
  DDRC   = _BV( 4 );
  PORTC |= _BV( 3 );

// normal PWM
  TCCR2A = _BV( WGM20) | _BV( WGM21) | _BV( COM2A1 ) | _BV( COM2A0 ) | _BV( COM2B1 ) | _BV( COM2B0 );
  TCCR2B = _BV( CS21 ) | _BV( CS21 ) | _BV( CS22 );

  uint8_t level = 0;

  sei();

  while (1) {
    level = level & 0x1f;
    OCR2A = levels[level];
    OCR2B = levels[0x1f-level];
    PORTC ^= _BV( 4 );

//    for (uint8_t i=0;i<3;i++)
      _delay_ms(400);

    if (!bit_is_set(PINC, 3))
      level++;
  }
}
