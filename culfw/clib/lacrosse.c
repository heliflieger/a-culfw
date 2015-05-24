/*
 * Copyright by D.Tostmann
 * inspired by LaCrosseITPlusReader
 * License: GPL v2
 */
#include "board.h"
#ifdef LACROSSE_HMS_EMU
#include "lacrosse.h"
#include <string.h>
#include "display.h"
#include <avr/pgmspace.h>

/*
* Message Format:
*
* .- [0] -. .- [1] -. .- [2] -. .- [3] -. .- [4] -.
* |       | |       | |       | |       | |       |
* SSSS.DDDD DDN_.TTTT TTTT.TTTT WHHH.HHHH CCCC.CCCC
* |  | |     ||  |  | |  | |  | ||      | |       |
* |  | |     ||  |  | |  | |  | ||      | `--------- CRC
* |  | |     ||  |  | |  | |  | |`-------- Humidity
* |  | |     ||  |  | |  | |  | |
* |  | |     ||  |  | |  | |  | `---- weak battery
* |  | |     ||  |  | |  | |  |
* |  | |     ||  |  | |  | `----- Temperature T * 0.1
* |  | |     ||  |  | |  |
* |  | |     ||  |  | `---------- Temperature T * 1
* |  | |     ||  |  |
* |  | |     ||  `--------------- Temperature T * 10
* |  | |     | `--- new battery
* |  | `---------- ID
* `---- START = 9
*
*/

static uint8_t CalculateCRC(uint8_t *data, uint8_t len) {
  uint8_t i, j;
  uint8_t res = 0;
  for (j = 0; j < len; j++) {
    uint8_t val = data[j];
    for (i = 0; i < 8; i++) {
      uint8_t tmp = (uint8_t)((res ^ val) & 0x80);
      res <<= 1;
      if (0 != tmp) {
        res ^= 0x31;
      }
      val <<= 1;
    }
  }
  return res;
}

void dec2hms_lacrosse(uint8_t * pl) {
  char    sign  = '0';
  uint8_t state = 0;
  
  if ((pl[0] & 0x90) != 0x90)
    return;

  if (CalculateCRC(pl,4) != pl[4])
    return;

  uint8_t ID = 0;
  ID |= (pl[0] & 0xF) << 2;
  // The new ID calculation. The order of the bits is respected
  ID |= (pl[1] & 0xC0) >> 6;

  if ((pl[1] & 0x20) >> 5)
    state |= 4; // new bat

  // uint8_t Bit12 = (pl[1] & 0x10) >> 4; // Dual temp type

  uint8_t bcd[3];
  bcd[0] = pl[1] & 0xF;
  bcd[1] = (pl[2] & 0xF0) >> 4;
  bcd[2] = (pl[2] & 0xF);

  double t = 0;
  t += bcd[0] * 100.0;
  t += bcd[1] * 10.0;
  t += bcd[2] * 1.0;
  //  t = t / 10;
  int temp = t - 400;

  if ((pl[3] & 0x80) >> 7)
    state |= 2; // bat empty

  uint8_t Humidity = pl[3] & 0b01111111;
  if (Humidity>=100) {

    // looks like a dual temp Sensor
    if (Humidity >= 125)
      ID |= _BV(6); // set highest bit in ID for differentiation
    
    Humidity = 0;
  }

  if (temp < 0 ) {
    state |= 8;
    temp*=-1;
  }
  
  DC('H');
  DH2('C'); // fix prefix because we need a 16bit ID as address
  DH2(ID);
  DU(state,1);
  DC( Humidity ? '0' : '1'); 		//HMS Type 
  //Temp under 10 degs
  sign  =  (char) ( ( (uint8_t) ((temp / 10 ) % 10) )&0x0F ) + '0';
  DC(sign);
  //Degrees below 1  
  sign  =  (char) ( ( (uint8_t)  (temp % 10 ) 	  )&0x0F ) + '0';
  DC(sign);
  DC('0');
  //Temp over 9 degs
  sign  =  (char) ( ( (uint8_t) ((temp / 100) % 10) )&0x0F ) + '0';
  DC(sign);
  display_udec(Humidity,2,'0');
  DC('F');DC('F');                                            // RSSI
  DNL();
  
}

#endif
