#include "board.h"
#include "cc1100.h"
#include <utility/trace.h>
#include "ethernet.h"
#include "delay.h"

#define FEATURE_ETHERNET    7

#ifdef USE_HW_AUTODETECT

static uint8_t hw_features =0;

void hw_autodetect(void) {

  for (CC1101.instance = 0; CC1101.instance < HAS_MULTI_CC; CC1101.instance++) {
    ccStrobe( CC1100_SRES );                   // Send SRES command
    my_delay_us(100);
  }

  for (CC1101.instance = 0; CC1101.instance < HAS_MULTI_CC; CC1101.instance++) {
    uint8_t pn, ver;
    pn = cc1100_readReg(CC1100_PARTNUM);
    ver = cc1100_readReg(CC1100_VERSION);
    if(pn == 0) {
      switch(ver & 0x0F) {
      case 0x03:
      case 0x04:
      case 0x05:
      case 0x07:
        hw_features |= 1<<CC1101.instance;
        TRACE_INFO("Detected CC%x: PN 0x%02x  VER 0x%02x \n\r", CC1101.instance,pn,ver);
        break;
      default:
        TRACE_INFO("Not detected CC%x: PN 0x%02x  VER 0x%02x \n\r", CC1101.instance,pn,ver);
      }
    } else {
      TRACE_INFO("Not detected CC%x: PN 0x%02x  VER 0x%02x \n\r", CC1101.instance,pn,ver);
    }
  }
  CC1101.instance = 0;

#if defined(HAS_WIZNET) || defined(HAS_ETHERNET)
#if defined(CUBE) || defined(CUBE_BL) || defined(CUBEx4) || defined(CUBEx4_BL)
  hw_features |= 1<<FEATURE_ETHERNET;
#else
  if (check_Net_MAC()) {
    hw_features |= 1<<FEATURE_ETHERNET;
    TRACE_INFO("Detected ethernet \n\r");
  } else {
    TRACE_INFO("Not detected ethernet \n\r");
  }
#endif
#endif

}

uint8_t has_CC(uint8_t num) {
  return hw_features & (1<<num);
}

uint8_t has_ethernet(void) {
#if defined(HAS_WIZNET) || defined(HAS_ETHERNET)
  return hw_features & (1<<FEATURE_ETHERNET);
#else
  return 0;
#endif
}

uint8_t get_hw_features(void) {
  return hw_features;
}
#endif
