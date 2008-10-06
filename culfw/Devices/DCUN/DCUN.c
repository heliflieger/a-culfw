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


/* Scheduler Task List */
TASK_LIST
{
  { Task: USB_USBTask  , TaskStatus: TASK_STOP },
  { Task: CDC_Task     , TaskStatus: TASK_STOP },
  { Task: RfAnalyze_Task,TaskStatus: TASK_RUN },
  { Task: Minute_Task,   TaskStatus: TASK_RUN },
};

t_fntab fntab[] = {

  { 'b', prepare_b },
  { 'B', prepare_B },
  { 'C', ccreg },
  { 'F', fs20send },
  { 'l', ledfunc },
  { 'R', read_eeprom },
  { 'T', fhtsend },
  { 't', gettime },
  { 'V', version },
  { 'W', write_eeprom },
  { 'X', set_txreport },
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

EVENT_HANDLER(USB_Connect) {
     Scheduler_SetTaskMode(USB_USBTask, TASK_RUN);
}

EVENT_HANDLER(USB_Disconnect) {
     PORTE |= _BV( PE1 );
     Scheduler_SetTaskMode(USB_USBTask, TASK_STOP);
     Scheduler_SetTaskMode(CDC_Task,    TASK_STOP);
     ccStrobe(CC1100_SIDLE);
}

int
main(void)
{

  // if we had been restarted by watchdog check the REQ BootLoader byte in the
  // EEPROM ...
  if (bit_is_set(MCUSR,WDRF) && eeprom_read_byte(EE_REQBL)) {
       eeprom_write_byte( EE_REQBL, 0 ); // clear flag
       start_bootloader();
  }

  MCUSR &= ~(1 << WDRF);

  DDRE  |= _BV( PE1 );
  PORTE |= _BV( PE1 );

  wdt_enable(WDTO_2S); 

  SetSystemClockPrescaler(0);                   // Disable Clock Division

  led_init();
  SPI_MasterInit(DATA_ORDER_MSB);
  
  rb_init(Tx_Buffer, TX_SIZE);
  credit_10ms = MAX_CREDIT/2;

  minute = 0;
  OCR0A  = 249;  // Timer0: 0.008s (1/125sec) = 8MHz/256/250  (?)
  TCCR0B = _BV(CS02);       
  TCCR0A = _BV(WGM01);
  TIMSK0 = _BV(OCIE0A);

  Scheduler_Init();                            

  USB_Init();

  Scheduler_Start();                            // Won't return
}

