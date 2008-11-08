#include <avr/io.h>
#include "board.h"
#include "display.h"
#include "delay.h"
#ifdef HAS_GLCD
#include "pcf8833.h"
#endif

//*****************************************************************************
// Function: get_adcw
// Parameters: none.
// Returns: none.
//
// Description: start AD conversion to read specific ADC channel
//*****************************************************************************

uint16_t
get_adcw(uint8_t mux)
{
  uint8_t i;
  uint16_t result = 0;
  
  // Frequenzvorteiler setzen auf 8 (1) und ADC aktivieren (1)
  ADCSRA = (1<<ADEN) | (1<<ADPS1) | (1<<ADPS0); 

  ADMUX = mux;                      // Kanal waehlen
  ADMUX |= (1<<REFS1) | (1<<REFS0); // interne Referenzspannung nutzen

  for(i=0; i<2; i++) {
    ADCSRA |= (1<<ADSC);              // Dummy readout
    while ( ADCSRA & (1<<ADSC) );     // wait for converter to finish
  }
  my_delay_ms(1);
  
  /* Eigentliche Messung - Mittelwert aus 4 aufeinanderfolgenden Wandlungen */
  for(i=0; i<4; i++) {
    ADCSRA |= (1<<ADSC);            // eine Wandlung "single conversion"
    while ( ADCSRA & (1<<ADSC) );   // auf Abschluss der Konvertierung warten
    result += ADCW;                 // Wandlungsergebnisse aufaddieren
  }
  ADCSRA &= ~(1<<ADEN);             // ADC deaktivieren (2)
  
  result /= 4; 
  
  return result;
}


void
batfunc(char *unused)
{
  uint8_t s1;
  s1  = (bit_is_set( BAT_PIN, BAT_PIN1) ? 2 : 0);
  s1 |= (bit_is_set( BAT_PIN, BAT_PIN2) ? 1 : 0);

  if(s1==0) DS_P( PSTR("battery mode"));
  if(s1==1) DS_P( PSTR("charged"));
  if(s1==2) DS_P( PSTR("charging"));
  if(s1==3) DS_P( PSTR("error"));

  DU(get_adcw(BAT_MUX),5);    // result, decimal
  DNL();
}


#if defined(HAS_GLCD) && defined(BAT_PIN)
void
bat_drawstate(void)
{
  if(!lcd_on)
    return;

  uint16_t v = get_adcw(BAT_MUX);
  if(v < 750) v = 750;
  if(v > 950) v = 950;

  v = (v-750)*VISIBLE_WIDTH/200;
  lcd_line(WINDOW_LEFT,TITLE_HEIGHT-1,                       // the green part
                   WINDOW_LEFT+v,TITLE_HEIGHT-1, 0x0f00);
  lcd_line(WINDOW_LEFT+v,TITLE_HEIGHT-1,                     // and the red part
                   WINDOW_RIGHT,TITLE_HEIGHT-1, 0xf000);
}
#endif

void
bat_init(void)
{
  BAT_DDR   = 0;
  BAT_PORT |= (_BV(BAT_PIN1) | _BV(BAT_PIN1));          // pull Battery stati
}
