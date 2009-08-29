#include <avr/eeprom.h>
#include <Drivers/USB/USB.h>

#include "board.h"
#include "display.h"
#include "delay.h"
#include "fncollection.h"
#include "cc1100.h"
#include "../version.h"
#include "cdc.h"
#include "clock.h"
#include "mysleep.h"
#include "fswrapper.h"
#include "fastrf.h"
#include "ethernet.h"

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

#ifdef HAS_ETHERNET
void
display_mac(uint8_t *a)
{
  uint8_t cnt = 6;
  while(cnt--) {
    DH2(erb(a++));
    if(cnt)
      DC(':');
  }
  
}

void
display_ip4(uint8_t *a)
{
  uint8_t cnt = 4;
  while(cnt--) {
    DU(erb(a++),1);
    if(cnt)
      DC('.');
  }
  
}
#endif

void
read_eeprom(char *in)
{
  uint8_t hb[2], d;
  uint16_t addr;

#ifdef HAS_ETHERNET
  if(in[1] == 'i') {
           if(in[2] == 'm') { display_mac(EE_MAC_ADDR);
    } else if(in[2] == 'd') { DH2(erb(EE_USE_DHCP));
    } else if(in[2] == 'a') { display_ip4(EE_IP4_ADDR);
    } else if(in[2] == 'n') { display_ip4(EE_IP4_NETMASK);
    } else if(in[2] == 'g') { display_ip4(EE_IP4_GATEWAY);
    } else if(in[2] == 'p') {
      DU(eeprom_read_word((uint16_t *)EE_IP4_TCPLINK_PORT), 0);
    }
  } else 
#endif
  {
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
    DH2(d);                    // result, hex
    DS_P( PSTR(" / ") );
    DU(d,2);                    // result, decimal
  }
  DNL();
}

void
write_eeprom(char *in)
{
  uint8_t hb[6], d = 0;

#ifdef HAS_ETHERNET
  if(in[1] == 'i') {
    uint8_t *addr = 0;
           if(in[2] == 'm') { d=6; fromhex(in+3,hb,6); addr=EE_MAC_ADDR;
    } else if(in[2] == 'd') { d=1; fromdec(in+3,hb);   addr=EE_USE_DHCP;
    } else if(in[2] == 'a') { d=4; fromip (in+3,hb,4); addr=EE_IP4_ADDR;
    } else if(in[2] == 'n') { d=4; fromip (in+3,hb,4); addr=EE_IP4_NETMASK;
    } else if(in[2] == 'g') { d=4; fromip (in+3,hb,4); addr=EE_IP4_GATEWAY;
    } else if(in[2] == 'p') { d=2; fromdec(in+3,hb);   addr=EE_IP4_TCPLINK_PORT;
    }
    for(uint8_t i = 0; i < d; i++)
      ewb(addr++, hb[i]);

  } else 
#endif
  {
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
#ifdef HAS_FASTRF
  fastrf_reset();
#endif

  ewb(EE_MAGIC_OFFSET  , VERSION_1);
  ewb(EE_MAGIC_OFFSET+1, VERSION_2);

  ewb(EE_REQBL, 0);
  ewb(EE_LED, 2);
#ifdef HAS_LCD
  ewb(EE_CONTRAST,   0x40);
  ewb(EE_BRIGHTNESS, 0x80);
  ewb(EE_SLEEPTIME, 30);
#endif
#ifdef HAS_ETHERNET
  ethernet_reset();
#endif
  prepare_boot(in);
}

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
#ifdef HAS_FS
  fs_sync(&fs);
#endif

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
#ifdef BUSWARE_CUL // check 433MHz version marker
  if (!bit_is_set(PINB, PB6))
    DS_P( PSTR("V " VERSION " " BOARD_ID_STR433) );
  else
#endif
  DS_P( PSTR("V " VERSION " " BOARD_ID_STR) );
  DNL();
}
