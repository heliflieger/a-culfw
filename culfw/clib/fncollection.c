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

// eeprom_write_byte is inlined and it is too big
__attribute__((__noinline__)) void
ewb(uint8_t *p, uint8_t v)
{
  eeprom_write_byte(p, v);
}

// eeprom_read_byte is inlined and it is too big
__attribute__((__noinline__)) uint8_t
erb(uint8_t *p)
{
  return eeprom_read_byte(p);
}

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

  d = erb((uint8_t *)addr);
  DC('R');                    // prefix
  DH(addr,4);                 // register number
  DS_P( PSTR(" = ") );
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
    
  ewb((uint8_t*)addr, hb[d-1]);

  // If there are still bytes left, then write them too
  in += (2*d+1);
  while(in[0]) {
    addr++;
    if(!fromhex(in, hb, 1))
      return;
    ewb((uint8_t*)addr++, hb[0]);
    in += 2;
  }
}

void
eeprom_init(void)
{
  if(erb(EE_MAGIC_OFFSET)   != VERSION_1 ||
     erb(EE_MAGIC_OFFSET+1) != VERSION_2)
    eeprom_factory_reset(0);

  led_mode = erb(EE_LED);
#ifdef HAS_SLEEP
  sleep_time = erb(EE_SLEEPTIME);
#endif
}

void
eeprom_factory_reset(char *in)
{
  cc_factory_reset();

  ewb(EE_MAGIC_OFFSET  , VERSION_1);
  ewb(EE_MAGIC_OFFSET+1, VERSION_2);

  ewb(EE_REQBL, 0);
  ewb(EE_LED, 2);
#ifdef HAS_LCD
  ewb(EE_CONTRAST,   0x40);
  ewb(EE_BRIGHTNESS, 0x80);
#endif
#ifdef HAS_SLEEP
  ewb(EE_SLEEPTIME, 30);
#endif
  prepare_boot(in);
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

  ewb(EE_LED, led_mode);
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
       ewb( EE_REQBL, 1 );

  TIMSK0 = 0;        // Disable the clock which resets the watchdog

  // go to bed, the wathchdog will take us to reset
  while (1);
}

void
version(char *unused)
{
  DS_P( PSTR("V " VERSION " " BOARD_ID_STR " ") );
  DH( cc1100_readReg( CC1100_PARTNUM ) & 0x0f, 1 );
  DH( cc1100_readReg( CC1100_VERSION ) & 0x0f, 1 );
  DNL();
}
