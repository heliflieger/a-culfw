/*
 * Copyright by D.Tostmann
 * This is a generic module for RFM12-based protocols
 * feel free to add mode modes by creating "overrrides"
 *
 * Payload is not check, just transferred - therefor 
 * you need to specify max RX Payload length in CC1100_FIFOTHR
 *
 * License: GPL v2
 */
#include <avr/io.h>                     // for _BV, bit_is_set
#include <stdint.h>                     // for uint8_t

#include "board.h"                      // for CC1100_CS_DDR, etc
#include "led.h"                        // for SET_BIT
#include "stringfunc.h"                 // for fromdec
#ifdef HAS_RFNATIVE
#include <avr/pgmspace.h>               // for pgm_read_byte, PROGMEM, etc

#include "cc1100.h"                     // for ccStrobe, CC1100_FIFOTHR, etc
#include "delay.h"                      // for my_delay_us, my_delay_ms
#include "display.h"                    // for DH2, DNL, DC, DS_P
#include "fband.h"                      // for checkFrequency
#include "rf_native.h"
#include "rf_mode.h"
#include "multi_CC.h"

#ifdef USE_HAL
#include "hal.h"
#endif

#ifndef USE_RF_MODE
static uint8_t native_on = 0;
#endif

#ifdef LACROSSE_HMS_EMU
uint8_t payload[5];
#include "lacrosse.h"
#endif

// This is the default - equals Mode 1
const uint8_t PROGMEM NATIVE_CFG[46] = {
  
  CC1100_FSCTRL1, 0x06,
  CC1100_FREQ2, 0x21,   // FREQ2     Frequency control word, high byte.
  CC1100_FREQ1, 0x65,   // FREQ1     Frequency control word, middle byte.
  CC1100_FREQ0, 0x6A,   // FREQ0     Frequency control word, low byte.
  
  CC1100_MCSM1, 0x00,   // always go into IDLE
  CC1100_MCSM0, 0x18,
  CC1100_FOCCFG, 0x16,
  CC1100_AGCCTRL2, 0x43,
  CC1100_AGCCTRL1, 0x68,
  CC1100_FSCAL1, 0x00,
  CC1100_FSCAL0, 0x11,
  
  CC1100_IOCFG2, 0x01,   // IOCFG2  RX FIFO at CC1100_FIFOTHR given byte count
  CC1100_IOCFG0, 0x46,   // IOCFG0  not used
  
  CC1100_SYNC0, 0xd4,
  CC1100_SYNC1, 0x2d,
  
  CC1100_FIFOTHR, 2,     // 12 byte in RX - see page 72 of CC1101.pdf
  
  CC1100_PKTCTRL1, 0x00,   // PKTCTRL1  Packet automation control.
  CC1100_PKTCTRL0, 0x02,   // PKTCTRL0  Packet automation control - infinite len
  
  CC1100_MDMCFG4, 0x89,   // MDMCFG4   Modem configuration.
  CC1100_MDMCFG3, 0x5C,   // MDMCFG3   Modem configuration.
  CC1100_MDMCFG2, 0x06,   // !! 05 !! MDMCFG2   Modem configuration.
  
  CC1100_DEVIATN, 0x56,   // DEVIATN   Modem deviation setting (when FSK modulation is enabled).
  
  0xff
};

#define MAX_MODES 3

// Here the overrides of the default for each mode:
const uint8_t PROGMEM MODE_CFG[MAX_MODES][20] = {
  // Mode 1 - IT+ 17.241 kbps
  {
    CC1100_FIFOTHR, 2,     // 12 byte in RX
    0xff,
  },
  // Mode 2 - IT+ 9.579 kbps
  {
    CC1100_FIFOTHR, 2,     // 12 byte in RX
    CC1100_MDMCFG4, 0x88, 
    CC1100_MDMCFG3, 0x82,
    0xff,
  },
  // Mode 3 - PCA 301 - 868.9500MHz 6.631kbps
  {
    CC1100_FIFOTHR, 0x07,   // 32 byte in RX
    CC1100_MDMCFG4, 0x88,   // MDMCFG4   Modem configuration.
    CC1100_MDMCFG3, 0x0B,   // MDMCFG3   Modem configuration.

    CC1100_FREQ2, 0x21,   // FREQ2     Frequency control word, high byte.
    CC1100_FREQ1, 0x6B,   // FREQ1     Frequency control word, middle byte.
    CC1100_FREQ0, 0xD0,   // FREQ0     Frequency control word, low byte.

    CC1100_DEVIATN, 0x53,   // DEVIATN   Modem deviation setting (when FSK modulation is enabled).

    0xff,
  },
};


void native_init(uint8_t mode) {

#ifdef USE_HAL
  hal_CC_GDO_init(CC_INSTANCE,INIT_MODE_IN_CS_IN);
  hal_enable_CC_GDOin_int(CC_INSTANCE,FALSE); // disable INT - we'll poll...
#else
  EIMSK &= ~_BV(CC1100_INT);                 // disable INT - we'll poll...
  SET_BIT( CC1100_CS_DDR, CC1100_CS_PIN );   // CS as output
#endif

#ifndef USE_RF_MODE
  native_on = 0;
#endif

  CC1100_DEASSERT;                           // Toggle chip select signal
  my_delay_us(30);
  CC1100_ASSERT;
  my_delay_us(30);
  CC1100_DEASSERT;
  my_delay_us(45);

  ccStrobe( CC1100_SRES );                   // Send SRES command
  my_delay_us(100);

  if (!mode || mode>MAX_MODES)
    return;
  
  // load configuration
  for (uint8_t i = 0; i<60; i += 2) {
       
    if (pgm_read_byte( &NATIVE_CFG[i] )>0x40)
      break;

    cc1100_writeReg( pgm_read_byte(&NATIVE_CFG[i]),
                     pgm_read_byte(&NATIVE_CFG[i+1]) );
  }

  // load special configuration
  for (uint8_t i = 0; i<20; i += 2) {
    
    if (pgm_read_byte( &MODE_CFG[mode-1][i] )>0x40)
      break;
    
    cc1100_writeReg( pgm_read_byte(&MODE_CFG[mode-1][i]),
                     pgm_read_byte(&MODE_CFG[mode-1][i+1]) );
  }

  
  ccStrobe( CC1100_SCAL );

#ifndef USE_RF_MODE
  native_on = mode;
#endif
  checkFrequency(); 
  my_delay_ms(1);
}

void native_task(void) {
  uint8_t len, byte, i;

#ifndef USE_RF_MODE
  if(!native_on)
    return;
#endif

  // wait for CC1100_FIFOTHR given bytes to arrive in FIFO:
#ifdef USE_HAL
  if (hal_CC_Pin_Get(CC_INSTANCE,CC_Pin_In)) {
#else
  if (bit_is_set( CC1100_IN_PORT, CC1100_IN_PIN )) {
#endif

    // start over syncing
    ccStrobe( CC1100_SIDLE );

    len = cc1100_readReg( CC1100_RXBYTES ) & 0x7f; // read len, transfer RX fifo
    
    if (len) {
      
      CC1100_ASSERT;
      cc1100_sendbyte( CC1100_READ_BURST | CC1100_RXFIFO );

      MULTICC_PREFIX();
      DC( 'N' );
#ifdef USE_RF_MODE
      DH2(get_RF_mode() - RF_mode_native1 + 1);
#else
      DH2(native_on);
#endif



      for (i=0; i<len; i++) {
         byte = cc1100_sendbyte( 0 );

#if defined(LACROSSE_HMS_EMU)
        if (i<sizeof(payload))
          payload[i] = byte;
#endif
        DH2( byte );
      }
      
      CC1100_DEASSERT;
      
      DNL();

#ifdef LACROSSE_HMS_EMU
      if (len>=5)
	dec2hms_lacrosse(payload);
#endif

    }

    return;
  }
       
  switch (cc1100_readReg( CC1100_MARCSTATE )) {
            
       // RX_OVERFLOW
  case 17:
       // IDLE
  case 1:
    ccStrobe( CC1100_SFRX  );
    ccStrobe( CC1100_SIDLE );
    ccStrobe( CC1100_SNOP  );
    ccStrobe( CC1100_SRX   );
    break;
       
  }
}


void native_func(char *in) {
#ifdef ARM
  uint16_t mode = 0;
#else
  uint8_t mode = 0;
#endif



  if(in[1] == 'r') {                // Reception on
    
    // "Er<x>" - where <x> is mode
    if (in[2])
      fromdec(in+2, ( uint8_t*)&mode);

    if (!mode || mode>MAX_MODES) {
      MULTICC_PREFIX();
      DS_P(PSTR("specify valid mode number\r\n"));
      return;
    }
    
#ifdef USE_RF_MODE
    set_RF_mode(RF_mode_native1 + mode - 1);
#else
    native_init(mode);
#endif

  } else if(in[1] == 'x') {        // Reception off

#ifdef USE_RF_MODE
    set_RF_mode(RF_mode_off);
#else
    if (native_on)
      ccStrobe( CC1100_SIDLE );
    
    native_on = 0;
#endif

  }
  MULTICC_PREFIX();
#ifdef USE_RF_MODE
  DH2(get_RF_mode() - RF_mode_native1 + 1);
#else
  DH2(native_on);
#endif
  DNL();
}

#endif


