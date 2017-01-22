#ifndef _MULTI_CC_H
#define _MULTI_CC_H

#include <stdint.h>

typedef enum {
  RF_mode_off = 0,
  RF_mode_slow,
  RF_mode_asksin,
  RF_mode_moritz,
  RF_mode_maico,
  RF_mode_native1,
  RF_mode_native2,
  RF_mode_native3,
  RF_mode_somfy,
  RF_mode_intertechno,
} RF_mode_t;

typedef struct {
  uint8_t     instance;
  RF_mode_t   RF_mode[HAS_MULTI_CC];
  RF_mode_t   RF_mode_save[HAS_MULTI_CC];
  uint8_t     tx_report[HAS_MULTI_CC];
} multiCC_t;

extern multiCC_t multiCC;

void multiCC_prefix(void);
void multiCC_func();

uint8_t is_changed_RF_mode(void);
uint8_t is_RF_mode(RF_mode_t mode);
void init_RF_mode(void);
void change_RF_mode(RF_mode_t mode);
void save_RF_mode(void);
void set_RF_mode(RF_mode_t mode);
RF_mode_t get_RF_mode(void);
uint8_t restore_RF_mode(void);

#endif
