#include "board.h"

#ifdef HAS_MULTI_CC

#include <stdint.h>

#include "multi_CC.h"
#include "display.h"
#include "ttydata.h"
#include "rf_receive.h"
#include "delay.h"
#include "cc1100.h"
#include "hal_gpio.h"

#ifdef HAS_ASKSIN
#include "rf_asksin.h"
#endif
#ifdef HAS_MORITZ
#include "rf_moritz.h"
#endif
#ifdef HAS_RWE
#include "rf_rwe.h"
#endif
#ifdef HAS_RFNATIVE
#include "rf_native.h"
#endif
#ifdef HAS_INTERTECHNO
#include "intertechno.h"
#endif
#ifdef HAS_SOMFY_RTS
#include "somfy_rts.h"
#endif
#ifdef HAS_MBUS
#include "rf_mbus.h"
#endif
#ifdef HAS_KOPP_FC
#include "kopp-fc.h"
#endif
#ifdef HAS_ZWAVE
#include "rf_zwave.h"
#endif
#ifdef HAS_MAICO
#include "rf_maico.h"
#endif

extern const t_fntab fntab[];
extern const t_fntab fntab1[];
extern const t_fntab fntab2[];
extern const t_fntab fntab3[];

const t_fntab* fntabs[] =  {
    fntab,
    fntab1,
#if HAS_MULTI_CC > 2
    fntab2,
#endif
#if HAS_MULTI_CC > 3
    fntab3,
#endif
};
multiCC_t multiCC;

void multiCC_prefix(void) {
  for(uint8_t x=0; x<multiCC.instance; x++) {
    DC( '*' );
  }
}

uint8_t callfn2(char *buf)
{
  for(uint8_t idx = 0; ; idx++) {
    uint8_t n = fntabs[ multiCC.instance][idx].name;
    void (*fn)(char *) = (void (*)(char *))fntabs[ multiCC.instance][idx].fn;
    if(!n)
      break;
    if(buf == 0) {
      DC(' ');
      DC(n);
    } else if(buf[0] == n) {
      fn(buf);
      return 1;
    }
  }
  return 0;
}

void multiCC_func(char *in) {

  in++;
  multiCC.instance++;

  if(!callfn2(in)) {
    multiCC_prefix();
    DS_P("? (");
    display_string(in);
    DS_P(" is unknown) Use one of");
    callfn2(0);
    DNL();
  }

  in--;
  multiCC.instance--;

}

void init_RF_mode(void) {
  for(uint8_t x=0; x<HAS_MULTI_CC; x++) {
    multiCC.RF_mode[x] = RF_mode_off;
    multiCC.RF_mode_save[x] = RF_mode_off;
    multiCC.tx_report[x] = 0;
    hal_CC_GDO_init(x,INIT_MODE_OUT_CS_IN);
  }
  multiCC.instance = 0;
}

uint8_t is_changed_RF_mode(void) {
  return multiCC.RF_mode_save[multiCC.instance] != multiCC.RF_mode[multiCC.instance];
}

uint8_t is_RF_mode(RF_mode_t mode) {
  return multiCC.RF_mode[multiCC.instance] == mode;
}

void change_RF_mode(RF_mode_t mode) {
  if(mode != multiCC.RF_mode[multiCC.instance]) {
    save_RF_mode();
    set_RF_mode(mode);
  }

}

void save_RF_mode(void) {
  multiCC.RF_mode_save[multiCC.instance] = multiCC.RF_mode[multiCC.instance];
}

void set_RF_mode(RF_mode_t mode) {

  multiCC.RF_mode[multiCC.instance] = mode;

  switch(mode) {
    case RF_mode_slow:
      set_txrestore();
      if(!multiCC.tx_report[multiCC.instance])
        multiCC.RF_mode[multiCC.instance] = RF_mode_off;
      break;
#ifdef HAS_ASKSIN
    case RF_mode_asksin:
      rf_asksin_init();
      my_delay_ms(3);
      break;
#endif
#ifdef HAS_MORITZ
    case RF_mode_moritz:
      rf_moritz_init();
      break;
#endif
#ifdef HAS_MAICO
    case RF_mode_maico:
      rf_maico_init();
      break;
#endif
#ifdef HAS_RFNATIVE
    case RF_mode_native1:
      native_init(1);
      break;
    case RF_mode_native2:
      native_init(2);
      break;
    case RF_mode_native3:
      native_init(3);
      break;
#endif
#ifdef HAS_SOMFY_RTS
    case RF_mode_somfy:
      somfy_rts_tunein();
      my_delay_ms(3);
      break;
#endif
#ifdef HAS_INTERTECHNO
    case RF_mode_intertechno:
      it_tunein();
      my_delay_ms(3);
      break;
#endif
    default:
      set_ccoff();
      multiCC.RF_mode[multiCC.instance] = RF_mode_off;
  }

}

RF_mode_t get_RF_mode(void) {
  return multiCC.RF_mode[multiCC.instance];
}

uint8_t restore_RF_mode(void) {
  if(multiCC.RF_mode_save[multiCC.instance] != multiCC.RF_mode[multiCC.instance]) {
    set_RF_mode(multiCC.RF_mode_save[multiCC.instance]);
    return 1;
  }
  return 0;
}

#endif
