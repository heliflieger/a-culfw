/* Copyright Rudolf Koenig, 2008.
   Released under the GPL Licence, Version 2
   Inpired by the MyUSB USBtoSerial demo, Copyright (C) Dean Camera, 2008.
*/

#include <avr/io.h>
#include <avr/boot.h>
#include <avr/eeprom.h>
#include <string.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include <MyUSB/Drivers/USB/USB.h>     // USB Functionality
#include <MyUSB/Scheduler/Scheduler.h> // Simple scheduler for task management
#include <Drivers/AT90USB162/SPI.h>    // SPI drivers

#include "cc1100.h"
#include "cdc.h"
#include "clock.h"
#include "delay.h"
#include "display.h"
#include "fncollection.h"
#include "led.h"
#include "ringbuffer.h"
#include "transceiver.h"
#include "ttydata.h"
#include "pcf8833.h"
#include "spi.h"
#include "battery.h"
#include "fswrapper.h"
#include "ttydata.h"
#include "joy.h"
#include "menu.h"
#include "mysleep.h"
#include "spi.h"
#include "ds1339.h"

df_chip_t df;

/* Scheduler Task List */
TASK_LIST
{
  { Task: USB_USBTask  , TaskStatus: TASK_STOP },
  { Task: CDC_Task     , TaskStatus: TASK_STOP },
  { Task: RfAnalyze_Task,TaskStatus: TASK_RUN },
  { Task: Minute_Task,   TaskStatus: TASK_RUN },
  { Task: JOY_Task,      TaskStatus: TASK_RUN },
};


t_fntab fntab[] = {

  { 'B', prepare_boot },
  { 'C', ccreg },
  { 'F', fs20send },
  { 'R', read_eeprom },
  { 'T', fhtsend },
  { 'V', version },
  { 'W', write_eeprom },
  { 'X', set_txreport },

  { 'a', batfunc },
  { 'c', rtcfunc },
  { 'e', eeprom_factory_reset },
  { 'd', lcdfunc },
  { 'l', ledfunc },
  { 'r', read_file },
  { 's', mysleep },
  { 't', gettime },
  { 'w', write_file },
  { 'x', ccsetpa },

  { 0, 0 },
};


#if defined(__AVR_AT90USB1286__)
#define jump_to_bootloader ((void(*)(void))0xf000)
#else
#define jump_to_bootloader ((void(*)(void))0x3000)
#endif

void
start_bootloader(void)
{
  cli();
  
  /* move interrupt vectors to bootloader section and jump to bootloader */
  MCUCR = _BV(IVCE);
  MCUCR = _BV(IVSEL);

  jump_to_bootloader();
}

EVENT_HANDLER(USB_Connect)
{
  Scheduler_SetTaskMode(USB_USBTask, TASK_RUN);
}

EVENT_HANDLER(USB_Disconnect)
{
  Scheduler_SetTaskMode(USB_USBTask, TASK_STOP);
  Scheduler_SetTaskMode(CDC_Task,    TASK_STOP);
  ccStrobe(CC1100_SIDLE);
}

int
main(void)
{
  spi_init();
  eeprom_init();

  // if we had been restarted by watchdog check the REQ BootLoader byte in the
  // EEPROM ...
  if (bit_is_set(MCUSR,WDRF) && eeprom_read_byte(EE_REQBL)) {
     eeprom_write_byte( EE_REQBL, 0 ); // clear flag
     start_bootloader();
  }


  // Setup the timers. Are needed for watchdog-reset
  OCR0A  = 249;                            // Timer0: 0.008s = 8MHz/256/250
  TCCR0B = _BV(CS02);       
  TCCR0A = _BV(WGM01);
  TIMSK0 = _BV(OCIE0A);

  TCCR1A = 0;
  TCCR1B = _BV(CS11) | _BV(WGM12);         // Timer1: 1us = 8MHz/8

  SetSystemClockPrescaler(0);              // Disable Clock Division

  MCUSR &= ~(1 << WDRF);                   // Enable the watchdog
  wdt_enable(WDTO_2S); 

  led_init();
  rb_init(USB_Tx_Buffer, CDC_TX_EPSIZE);
  rb_init(USB_Rx_Buffer, CDC_RX_EPSIZE);
  Scheduler_Init();                            
  USB_Init();
  tty_init();

  joy_init();
  bat_init();
  df_init(&df);
  fs_init(&fs, df);
  menu_init();
  rtc_init();
  LED_OFF();

  credit_10ms = MAX_CREDIT/2;

  Scheduler_Start();                            // Won't return
}
