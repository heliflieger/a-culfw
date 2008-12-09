#include <avr/eeprom.h>
#include <MyUSB/Drivers/USB/USB.h>

#include "board.h"
#include "display.h"
#include "delay.h"
#include "fncollection.h"
#include "cc1100.h"
#include "version.h"
#include "cdc.h"
#include "transceiver.h"
#include "clock.h"

#ifdef HAS_LCD
#include "battery.h"
#include "pcf8833.h"
#include "clock.h"
#include "mysleep.h"
#endif

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

  // If there are still bytes left, then write them too
  in += (2*d+1);
  while(in[0]) {
    addr++;
    if(!fromhex(in, hb, 1))
      return;
    eeprom_write_byte((uint8_t*)addr++, hb[0]);
    in += 2;
  }
}

void
eeprom_init(void)
{
  if(eeprom_read_byte(EE_MAGIC_OFFSET)   != EE_MAGIC ||
     eeprom_read_byte(EE_VERSION_OFFSET) != EE_VERSION)
    eeprom_factory_reset(0);

  led_mode = eeprom_read_byte(EE_LED);
#ifdef HAS_SLEEP
  sleep_time = eeprom_read_byte(EE_SLEEPTIME);
#endif
}

void
eeprom_factory_reset(char *unused)
{
  cc_factory_reset();

  eeprom_write_byte(EE_MAGIC_OFFSET, EE_MAGIC);
  eeprom_write_byte(EE_VERSION_OFFSET, EE_VERSION);

  eeprom_write_byte(EE_REQBL, 0);
  eeprom_write_byte(EE_LED, 2);
#ifdef HAS_LCD
  eeprom_write_byte(EE_CONTRAST,   0x40);
  eeprom_write_byte(EE_BRIGHTNESS, 0x80);
#endif
#ifdef HAS_SLEEP
  eeprom_write_byte(EE_SLEEPTIME, 30);
#endif
}

//////////////////////////////////////////////////
// LED
void
ledfunc(char *in)
{
  fromhex(in+1, &led_mode, 1);
  if(led_mode & 1)
    LED_ON();
  else
    LED_OFF();
  eeprom_write_byte(EE_LED, led_mode);
}

//////////////////////////////////////////////////
// boot
void
prepare_boot(char *in)
{
  uint8_t bl = 0;
  if(in)
    fromhex(in+1, &bl, 1);

  USB_ShutDown();

  // next reboot we like to jump to Bootloader ...
  if(bl)
       eeprom_write_byte( EE_REQBL, 1 );

  TIMSK0 = 0;        // Disable the clock which resets the watchdog

  // go to bed, the wathchdog will take us to reset
  while (1);
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

//////////////////////////////////////////////////
// LCD Monitor
#ifdef HAS_LCD
void
monitor(char *in)
{
  uint8_t a;
  fromhex(in+1, &a, 1);

  if(a) {
    display_channels |= DISPLAY_LCD;
    set_txreport("X21");
    batfunc(0);
  } else {
    display_channels &= ~DISPLAY_LCD;
    set_txreport("X00");
  }
}
#endif
