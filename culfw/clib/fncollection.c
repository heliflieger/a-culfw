#include <avr/eeprom.h>
#include <avr/wdt.h>
#include <MyUSB/Drivers/USB/USB.h>

#include "display.h"
#include "delay.h"
#include "fncollection.h"
#include "cc1100.h"
#include "version.h"

uint8_t led_mode = 2;   // Start blinking

//////////////////////////////////////////////////
// EEprom
void
read_eeprom(char *in)
{
  uint8_t hb[2], d;
  uint16_t addr;

  hb[0] = hb[2] = 0;
  d = fromhex(in+1, hb, 2);
  if(d == 2)
    addr = (hb[0] << 8) | hb[1];
  else
    addr = hb[0];

  d = eeprom_read_byte((uint8_t *)addr);
  DC('R');                    // prefix
  DH(addr,4);                 // register number
  DS_P( PSTR(" = 0x") );
  DH(d,2);                    // result, hex
  DS_P( PSTR(" / ") );
  DU(d,2);                    // result, decimal
  DNL();
}

void
write_eeprom(char *in)
{
  uint8_t hb[3], d;
  uint16_t addr;

  d = fromhex(in+1, hb, 3);
  if(d < 2)
    return;
  if(d == 2)
    addr = hb[0];
  else
    addr = (hb[0] << 8) | hb[1];
    
  eeprom_write_byte((uint8_t*)addr, hb[d-1]);
  if(addr <= 0x28)
    ccInitChip();
}


void
ledfunc(char *in)
{
  fromhex(in+1, &led_mode, 1);
  if(led_mode & 1)
    LED_ON();
  else
    LED_OFF();
}

void
prepare_boot( uint8_t bl )
{

     USB_ShutDown();

     // next reboot we like to jump to Bootloader ...
     if(bl)
	  eeprom_write_byte( EE_REQBL, 1 );

     TIMSK0 = 0;        // Disable the clock which resets the watchdog

     // go to bed, the wathchdog will take us to reset
     while (1);

}

void
prepare_b(char *unused)
{
  prepare_boot(0);
}

void
prepare_B(char *unused)
{
  prepare_boot(1);
}

void
version(char *unused)
{
  DS_P( PSTR("V " VERSION " * HW: ") );
  DH( cc1100_readReg( CC1100_PARTNUM ), 2 );
  DC( '.' );
  DH( cc1100_readReg( CC1100_VERSION ), 2 );
  DC( ' ' );
  DS_P( PSTR(BOARD_ID_STR) );
  DNL();
}
