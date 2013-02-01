#ifndef _MAIN_H
#define _MAIN_H

#include "../board.h"

#define LED_DDR                 DDRD
#define LED_PORT                PORTD
#define LED_PIN                 7

#define RELA_DDR                DDRD
#define RELA_PORT               PORTD
#define RELA_PIN                6

#define RELB_DDR                DDRA
#define RELB_PORT               PORTA
#define RELB_PIN                4

#define KEY_DDR                 DDRB
#define KEY_PORT                PORTB
#define KEY_PIN                 2
#define KEY_IN                  PINB

#define INPA_DDR                DDRA
#define INPA_PORT               PORTA
#define INPA_PIN                5
#define INPA_IN                 PINA

#define INPB_DDR                DDRC
#define INPB_PORT               PORTC
#define INPB_PIN                0
#define INPB_IN                 PINC

#endif

