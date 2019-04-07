#include "board.h"
#include "cc1100.h"
#include <utility/trace.h>
#include "ethernet.h"
#include "delay.h"

#ifdef HAS_ONEWIRE
#include "onewire.h"
#include "i2cmaster.h"
#endif

#define FEATURE_ONEWIRE     6
#define FEATURE_ETHERNET    7

#ifdef USE_HW_AUTODETECT

static uint8_t hw_features =0;


int
detect_ds2482(void)
{
  unsigned char ret;
  unsigned char status;

  i2c_init();

  ret = i2c_start(DS2482_I2C_ADDR+I2C_WRITE);
  ret |= i2c_write(DS2482_CMD_SRP);
  ret |= i2c_write(DS2482_READPTR_SR);

  i2c_stop(); //release bus, as this was just an initialization
  ret |= i2c_start(DS2482_I2C_ADDR+I2C_READ);

  status = i2c_readNak();
  i2c_stop();

  if(ret)
    return 0;

  if(status)
    return 1;

  return 0;
}


void hw_autodetect(void) {

  for (CC1101.instance = 0; CC1101.instance < HAS_MULTI_CC; CC1101.instance++) {
    hal_CC_GDO_init(CC1101.instance,INIT_MODE_IN_CS_IN);
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
      case 0x08:
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


  uint8_t numCC = 0;

  for(uint8_t i=0; i< HAS_MULTI_CC; i++) {
    if(hw_features & (1<<i)) {
      numCC += 1;
    } else {
      for(uint8_t j=i; j < HAS_MULTI_CC;j++) {
        if(hw_features & (1<<j)) {
          hal_CC_move_transceiver_pins(j,numCC);
          hw_features |= 1<<numCC;
          hw_features &= ~(1<<j);
          numCC += 1;
          break;
        }
      }
    }
  }



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

#ifdef HAS_ONEWIRE
  uint8_t i;
  for(i=numCC; i< HAS_MULTI_CC; i++) {
    hal_I2C_Set_Port(i);
    if(detect_ds2482()) {
     hw_features |= 1<<FEATURE_ONEWIRE;
     TRACE_INFO("Detected onewire \n\r");
     break;
    }
  }

  if(i>=HAS_MULTI_CC) {
     TRACE_INFO("Not detected onewire \n\r");
  }
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

uint8_t has_onewire(void) {
#if defined(HAS_ONEWIRE)
  return hw_features & (1<<FEATURE_ONEWIRE);
#else
  return 0;
#endif
}

uint8_t get_hw_features(void) {
  return hw_features;
}
#endif
