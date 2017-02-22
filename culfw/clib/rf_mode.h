#ifndef _RF_MODE_H
#define _RF_MODE_H

#include <stdint.h>
#include "board.h"

#ifdef USE_RF_MODE

typedef enum {
  RF_mode_off = 0,
  RF_mode_slow,
  RF_mode_asksin,
  RF_mode_moritz,
  RF_mode_WMBUS_S,
  RF_mode_WMBUS_T,
  RF_mode_maico,
  RF_mode_native1,
  RF_mode_native2,
  RF_mode_native3,
  RF_mode_somfy,
  RF_mode_intertechno,
  RF_mode_rwe,
  RF_mode_fast,
  RF_mode_zwave,

} RF_mode_t;

#ifdef HAS_MULTI_CC

#define CC_INSTANCE       CC1101.instance
#define TX_REPORT         (CC1101.tx_report[CC1101.instance])

typedef struct {
  uint8_t     instance;
  RF_mode_t   RF_mode[HAS_MULTI_CC];
  RF_mode_t   RF_mode_save[HAS_MULTI_CC];
  uint8_t     tx_report[HAS_MULTI_CC];
  uint8_t     frequencyMode[HAS_MULTI_CC];
} CC1101_t;

#else

#define CC_INSTANCE       0
#define TX_REPORT         (CC1101.tx_report[0])

typedef struct {
  uint8_t     instance;
  RF_mode_t   RF_mode[1];
  RF_mode_t   RF_mode_save[1];
  uint8_t     tx_report[1];
  uint8_t     frequencyMode[1];
} CC1101_t;

#endif

extern CC1101_t CC1101;

uint8_t is_changed_RF_mode(void);
uint8_t is_RF_mode(RF_mode_t mode);
void init_RF_mode(void);
void change_RF_mode(RF_mode_t mode);
void save_RF_mode(void);
void set_RF_mode(RF_mode_t mode);
RF_mode_t get_RF_mode(void);
uint8_t restore_RF_mode(void);
void rf_mode_task(void);

#else //USE_RF_MODE

#define CC_INSTANCE       0
#define TX_REPORT         (tx_report)

#endif //USE_RF_MODE
#endif
