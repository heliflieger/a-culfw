#include "board.h"
#include <avr/io.h>
#include "delay.h"
#include "display.h"

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
  
  // Frequenzvorteiler setzen auf 64 == 125kHz (ADPS1/ADPS0) und ADC aktivieren (ADEN)
  ADCSRA = (1<<ADEN) | (1<<ADPS1) | (1<<ADPS2); 

  ADMUX = mux;                      // Kanal waehlen
  ADMUX |= (1<<REFS1) | (1<<REFS0); // interne Referenzspannung (2.56V) nutzen

  for(i=0; i<2; i++) {
    ADCSRA |= (1<<ADSC);              // Dummy readout
    while ( ADCSRA & (1<<ADSC) );     // wait for converter to finish
  }
  my_delay_ms(1);
  
  /* Eigentliche Messung - Mittelwert aus 4 aufeinanderfolgenden Wandlungen */
  for(i=0; i<2; i++) {
    ADCSRA |= (1<<ADSC);            // eine Wandlung "single conversion"
    while ( ADCSRA & (1<<ADSC) );   // auf Abschluss der Konvertierung warten
    result += ADCW;                 // Wandlungsergebnisse aufaddieren
  }
  ADCSRA &= ~(1<<ADEN);             // ADC deaktivieren (2)
  
  result /= 2; 
  
  return result;
}
