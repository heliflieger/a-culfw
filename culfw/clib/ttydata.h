#ifndef _TTYDATA_H_
#define _TTYDATA_H_

#include "ringbuffer.h"

typedef struct _fntab {
  unsigned char name;
  void (* const fn)(char *);
} t_fntab;

void analyze_ttydata(uint8_t channel);
uint8_t callfn(char *buf);

void (*input_handle_func)(uint8_t channel);
void (*output_flush_func)(void);

extern rb_t TTY_Tx_Buffer;
extern rb_t TTY_Rx_Buffer;

#endif
