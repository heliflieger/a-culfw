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
#include "glcd.h"
#include "spi.h"


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
  { 'L', lcdfunc },
  { 'R', read_eeprom },
  { 's', lcdscroll },
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
  // if we had been restarted by watchdog check the REQ BootLoader byte in the
  // EEPROM ...
  if (bit_is_set(MCUSR,WDRF) && eeprom_read_byte(EE_REQBL)) {
     eeprom_write_byte( EE_REQBL, 0 ); // clear flag
     start_bootloader();
  }
  MCUSR &= ~(1 << WDRF);
  wdt_enable(WDTO_2S); 

  SetSystemClockPrescaler(0);                   // Disable Clock Division

  led_init();
  //spi_init();
  SPI_MasterInit(DATA_ORDER_MSB);
  
  rb_init(Tx_Buffer, TX_SIZE);

  // Setup the timers
  OCR0A  = 249;  // Timer0: 0.008s (1/125sec) = 8MHz/256/250  (?)
  TCCR0B = _BV(CS02);       
  TCCR0A = _BV(WGM01);
  TIMSK0 = _BV(OCIE0A);

  TCCR1A = 0;
  TCCR1B = _BV(CS11) | _BV(WGM12); // 8MHz/8 -> 1MHz / 1us

  Scheduler_Init();                            
  USB_Init();

  // LCD

  // Joystick
  DDRE   = 0;
  DDRA   = 0;
  PORTE |= (_BV(PE2) | _BV(PE6) | _BV(PE7));
  PORTA |= (_BV(PA0) | _BV(PA1));

  // Battery
  PORTF |= (_BV(PF1) | _BV(PF2));

  Scheduler_Start();                            // Won't return
}
