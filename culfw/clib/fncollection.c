#include "board.h"                      // IWYU pragma: keep

#include <avr/eeprom.h>                 // for __eerd_byte_UNKNOWN, etc
#include <avr/interrupt.h>              // for cli
#include <avr/io.h>                     // for bit_is_set
#include <avr/pgmspace.h>               // for PSTR

#ifndef bit_is_set
#include <avr/sfr_defs.h>               // for bit_is_set
#endif
#include <stdint.h>                     // for uint8_t, uint16_t, int8_t

#include "../version.h"                 // for VERSION_1, VERSION_2, etc
#include "cc1100.h"                     // for cc_factory_reset
#include "cdc_uart.h"                   // for EE_write_baud
#include "display.h"                    // for DC, DS_P, DH2, DNL, DU, DH
#include "fband.h"                      // for checkFrequency, IS433MHZ
#include "fncollection.h"
#include "led.h"                        // for LED_OFF, LED_ON
#include "qfs.h"                        // for fs_sync
#include "stringfunc.h"                 // for fromhex, fromip, fromdec
#include "hw_autodetect.h"
#ifdef HAS_USB
#ifdef SAM7
#include <usb/device/cdc-serial/CDCDSerialDriver.h>
#include <utility/trace.h>
#include "delay.h"
#elif defined STM32
#include "usb_device.h"
#else
#include <Drivers/USB/LowLevel/LowLevel.h>  // for USB_ShutDown
#endif
#endif
#include <avr/wdt.h>                    // for WDTO_15MS, wdt_enable

#include "ethernet.h"                   // for ethernet_reset
#include "fswrapper.h"                  // for fs
#include "mysleep.h"                    // for sleep_time
#include "multi_CC.h"

uint8_t led_mode = 2;   // Start blinking

#ifdef XLED
#include "xled.h"
#endif

#if defined(CDC_COUNT) && CDC_COUNT > 1
#include "cdc_uart.h"                   // for EE_write_baud
#endif

#if defined STM32
#define RTC_BOOTLOADER_FLAG 0x424C
#define RTC_BOOTLOADER_JUST_UPLOADED 0x424D
#include <stm32f103xb.h>
#endif
//////////////////////////////////////////////////
// EEprom

// eeprom_write_byte is inlined and it is too big
__attribute__((__noinline__)) 
void
ewb(uint8_t *p, uint8_t v)
{
  eeprom_write_byte(p, v);
  eeprom_busy_wait();
}

// eeprom_read_byte is inlined and it is too big
__attribute__((__noinline__)) 
uint8_t
erb(uint8_t *p)
{
  return eeprom_read_byte(p);
}

static void
display_ee_bytes(uint8_t *a, uint8_t cnt)
{
  while(cnt--) {
    DH2(erb(a++));
    if(cnt)
      DC(':');
  }

}

static void
display_ee_mac(uint8_t *a)
{
  display_ee_bytes( a, 6 );
}

#if defined(HAS_ETHERNET) | defined(HAS_WIZNET)
static void
display_ee_ip4(uint8_t *a)
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

#if defined(HAS_ETHERNET) | defined(HAS_WIZNET)
  if(in[1] == 'i') {
           if(in[2] == 'm') { display_ee_mac(EE_MAC_ADDR);
    } else if(in[2] == 'd') { DH2(erb(EE_USE_DHCP));
    } else if(in[2] == 'a') { display_ee_ip4(EE_IP4_ADDR);
    } else if(in[2] == 'n') { display_ee_ip4(EE_IP4_NETMASK);
    } else if(in[2] == 'g') { display_ee_ip4(EE_IP4_GATEWAY);
    } else if(in[2] == 'N') { display_ee_ip4(EE_IP4_NTPSERVER);
    } else if(in[2] == 'o') { DH2(erb(EE_IP4_NTPOFFSET));
    } else if(in[2] == 'p') {
      DU(eeprom_read_word((uint16_t *)EE_IP4_TCPLINK_PORT), 0);
    }
  } else 
#endif
  if(in[1] == 'M') { display_ee_mac(EE_DUDETTE_MAC); }  
  else if(in[1] == 'P') { display_ee_bytes(EE_DUDETTE_PUBL, 16); }  
  else {
    hb[0] = hb[1] = 0;
    d = fromhex(in+1, hb, 2);
    if(d == 2)
      addr = (hb[0] << 8) | hb[1];
    else
      addr = hb[0];

#ifdef HAS_MULTI_CC
#if NUM_SLOWRF > 1
    if(CC1101.instance == 1) {
      if((addr >= (uint32_t)EE_CC1100_CFG) && (addr < (uint32_t)EE_CC1100_CFG + EE_CC1100_CFG_SIZE)) {
        addr = addr - (uint32_t)EE_CC1100_CFG + (uint32_t)EE_CC1100_CFG1;
      }
    }
#endif
#if NUM_SLOWRF > 2
    if(CC1101.instance == 2) {
      if((addr >= (uint32_t)EE_CC1100_CFG) && (addr < (uint32_t)EE_CC1100_CFG + EE_CC1100_CFG_SIZE)) {
        addr = addr - (uint32_t)EE_CC1100_CFG + (uint32_t)EE_CC1100_CFG2;
      }
    }
#endif
#endif

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

#if defined(HAS_ETHERNET) | defined(HAS_WIZNET)
  if(in[1] == 'i') {
    uint8_t *addr = 0;
           if(in[2] == 'm') { d=6; fromhex(in+3,hb,6); addr=EE_MAC_ADDR;
    } else if(in[2] == 'd') { d=1; fromdec(in+3,hb);   addr=EE_USE_DHCP;
    } else if(in[2] == 'a') { d=4; fromip (in+3,hb,4); addr=EE_IP4_ADDR;
    } else if(in[2] == 'n') { d=4; fromip (in+3,hb,4); addr=EE_IP4_NETMASK;
    } else if(in[2] == 'g') { d=4; fromip (in+3,hb,4); addr=EE_IP4_GATEWAY;
    } else if(in[2] == 'p') { d=2; fromdec(in+3,hb);   addr=EE_IP4_TCPLINK_PORT;
    } else if(in[2] == 'N') { d=4; fromip (in+3,hb,4); addr=EE_IP4_NTPSERVER;
    } else if(in[2] == 'o') { d=1; fromhex(in+3,hb,1); addr=EE_IP4_NTPOFFSET;
#ifdef HAS_NTP
      extern int8_t ntp_gmtoff;
      ntp_gmtoff = hb[0];
#endif
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

#ifdef HAS_MULTI_CC
#if NUM_SLOWRF > 1
    if(CC1101.instance == 1) {
      if((addr >= (uint32_t)EE_CC1100_CFG) && (addr < (uint32_t)EE_CC1100_CFG + EE_CC1100_CFG_SIZE)) {
        addr = addr - (uint32_t)EE_CC1100_CFG + (uint32_t)EE_CC1100_CFG1;
      }
    }
#endif
#if NUM_SLOWRF > 2
    if(CC1101.instance == 2) {
      if((addr >= (uint32_t)EE_CC1100_CFG) && (addr < (uint32_t)EE_CC1100_CFG + EE_CC1100_CFG_SIZE)) {
        addr = addr - (uint32_t)EE_CC1100_CFG + (uint32_t)EE_CC1100_CFG2;
      }
    }
#endif
#endif

    ewb((uint8_t*)addr, hb[d-1]);

    if (addr == 15 || addr == 16 || addr == 17)
      checkFrequency();

    // If there are still bytes left, then write them too
    in += (2*d+1);
    while(in[0]) {
      if(!fromhex(in, hb, 1))
        return;
      ewb((uint8_t*)++addr, hb[0]);
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
#ifdef XLED
  switch (led_mode) {
    case 0:
      xled_pattern = 0;
      break;
    case 1:
      xled_pattern = 0xffff;
      break;
    case 2:
      xled_pattern = 0xff00;
      break;
    case 3:
      xled_pattern = 0xa000;
      break;
    case 4:
      xled_pattern = 0xaa00;
      break;
  }
#endif
#ifdef HAS_SLEEP
  sleep_time = erb(EE_SLEEPTIME);
#endif
}

void
eeprom_factory_reset(char *in)
{

  ewb(EE_MAGIC_OFFSET  , VERSION_1);
  ewb(EE_MAGIC_OFFSET+1, VERSION_2);

  cc_factory_reset();
  checkFrequency();

  ewb(EE_RF_ROUTER_ID, 0);
  ewb(EE_RF_ROUTER_ROUTER, 0);
  ewb(EE_REQBL, 0);
  ewb(EE_LED, 2);

#ifdef HAS_LCD
  ewb(EE_CONTRAST,   0x40);
  ewb(EE_BRIGHTNESS, 0x80);
  ewb(EE_SLEEPTIME, 30);
#endif
#if defined(HAS_ETHERNET) | defined(HAS_WIZNET)
  ethernet_reset();
#endif
#ifdef HAS_FS
  ewb(EE_LOGENABLED, 0x00);
#endif
#ifdef HAS_RF_ROUTER
  ewb(EE_RF_ROUTER_ID, 0x00);
  ewb(EE_RF_ROUTER_ROUTER, 0x00);
#endif

#ifdef HAS_WIZNET
  EE_write_baud(0,CDC_BAUD_RATE);
  EE_write_baud(1,CDC_BAUD_RATE);
#if CDC_COUNT > 3
  EE_write_baud(2,CDC_BAUD_RATE);
#endif  
#endif

  if(in[1] != 'x')
    prepare_boot(0);
}

// LED
void
ledfunc(char *in)
{
  fromhex(in+1, &led_mode, 1);
#ifdef XLED
  switch (led_mode) {
    case 0:
      xled_pattern = 0; 
      break;
    case 1:
      xled_pattern = 0xffff; 
      break;
    case 2:
      xled_pattern = 0xff00; 
      break;
    case 3:
      xled_pattern = 0xa000; 
      break;
    case 4:
      xled_pattern = 0xaa00; 
      break;
  }
#else
  if(led_mode & 1)
    LED_ON();
  else
    LED_OFF();
#endif

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

  if(bl == 0xff)             // Allow testing
    while(1);
    
#ifdef SAM7

	unsigned char volatile * const ram = (unsigned char *) AT91C_ISRAM;

	//Reset
	if(bl)
		*ram = 0xaa;		// Next reboot we'd like to jump to the bootloader.

	USBD_Disconnect();
	my_delay_ms(250);
	my_delay_ms(250);
	my_delay_ms(250);
	my_delay_ms(250);
	AT91C_BASE_RSTC->RSTC_RCR = AT91C_RSTC_PROCRST | AT91C_RSTC_PERRST | AT91C_RSTC_EXTRST   | 0xA5<<24;
	while (1);

#elif defined STM32
	USBD_Disconnect();

	RCC->APB1ENR |= (RCC_APB1ENR_PWREN | RCC_APB1ENR_BKPEN);
	    PWR->CR |= PWR_CR_DBP;
	if(bl) {
	  // Next reboot we'd like to jump to the bootloader.
    BKP->DR10 = RTC_BOOTLOADER_FLAG;
	} else {
    BKP->DR10 = RTC_BOOTLOADER_JUST_UPLOADED;
	}
	PWR->CR &=~ PWR_CR_DBP;

	wdt_enable(WDTO_2S);

	while (1);                 // go to bed, the wathchdog will take us to reset
#else
  if(bl)                     // Next reboot we'd like to jump to the bootloader.
    ewb( EE_REQBL, 1 );      // Simply jumping to the bootloader from here
                             // wont't work. Neither helps to shutdown USB
                             // first.
                             
#ifdef HAS_USB
  USB_ShutDown();            // ??? Needed?
#endif
#ifdef HAS_FS
  fs_sync(&fs);              // Sync the filesystem
#endif


  TIMSK0 = 0;                // Disable the clock which resets the watchdog
  cli();
  
  wdt_enable(WDTO_15MS);       // Make sure the watchdog is running 
  while (1);                 // go to bed, the wathchdog will take us to reset
#endif
}

void
version(char *in)
{
  MULTICC_PREFIX();
#if defined(CUL_HW_REVISION)
  if (in[1] == 'H') {
    DS_P( PSTR(CUL_HW_REVISION) );
    DNL();
    return;
  }
#endif
  DS_P( PSTR("V " VERSION " " FW_NAME " Build: " BUILD_NUMBER " (" BUILD_DATE ") "));
#ifdef MULTI_FREQ_DEVICE     // check 433MHz version marker
  if (!bit_is_set(MARK433_PIN, MARK433_BIT))
     DS_P( PSTR(BOARD_ID_STR433) );
  else
#endif
    DS_P( PSTR(BOARD_ID_STR) );
#ifdef USE_HW_AUTODETECT
  DC('_');
  DH2(get_hw_features());
#endif
  if (IS433MHZ) {
     DS_P( PSTR(" (F-Band: 433MHz)") );
  } else {
     DS_P( PSTR(" (F-Band: 868MHz)") );
  }
#ifdef HAS_I2CSLAVE
		DS_P( PSTR(" (I2C: 0x") );
		display_hex2(i2cSlaveAddr);
		DS_P( PSTR(")") );
#endif	
  DNL();
}

void
dumpmem(uint8_t *addr, uint16_t len)
{
  for(uint16_t i = 0; i < len; i += 16) {
    uint8_t l = len;
    if(l > 16)
      l = 16;
    DH(i,4);
    DC(':'); DC(' ');
    for(uint8_t j = 0; j < l; j++) {
      DH2(addr[j]);
      if(j&1)
        DC(' ');
    }
    DC(' ');
    for(uint8_t j = 0; j < l; j++) {
      if(addr[j] >= ' ' && addr[j] <= '~')
        DC(addr[j]);
      else
        DC('.');
    }
    addr += 16;
    DNL();
  }
  DNL();
}
