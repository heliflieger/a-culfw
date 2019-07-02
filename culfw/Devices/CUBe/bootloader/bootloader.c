/* Copyright Telekatz, 2015.
   Released under the GPL Licence, Version 2
   Inpired by the USB CDC Serial Project, Copyright (c) 2008, Atmel Corporation
*/

//------------------------------------------------------------------------------
//         Headers
//------------------------------------------------------------------------------

#include "board.h"
#include <pio/pio.h>
#include "hal_gpio.h"

#include <dbgu/dbgu.h>
#include <stdio.h>
#include <pio/pio_it.h>
#include <pit/pit.h>
#include <aic/aic.h>
#include <utility/trace.h>

#include <usb/common/core/USBConfigurationDescriptor.h>
#include <usb/device/core/USBD.h>
#include <usb/device/massstorage/MSDDriver.h>
#include <usb/device/massstorage/MSDLun.h>
#include <usb/device/core/USBDCallbacks.h>
#include <avr/wdt.h>
#include <led.h>
#include "ringbuffer.h"
#include "flash/flashd.h"
#include <utility/assert.h>
#include <avr/wdt.h>

//------------------------------------------------------------------------------
//         Local definitions
//------------------------------------------------------------------------------
//Cubeloader Version
#define VERSION 	"2.00"

/// PIT period value in µseconds.
#define PIT_PERIOD          1000

/// Use for power management
#define STATE_IDLE    0
/// The USB device is in suspend state
#define STATE_SUSPEND 4
/// The USB device is in resume state
#define STATE_RESUME  5
/// The USB device is in resume state
#define STATE_RX	  6

/// Size of one block in bytes.
#define BLOCK_SIZE          512

/// Maximum number of LUNs which can be defined.
#define MAX_LUNS            1

//------------------------------------------------------------------------------
//         Local variables
//------------------------------------------------------------------------------

/// Global timestamp in milliseconds since start of application.
volatile unsigned int timestamp = 0;

/// State of USB, for suspend and resume
unsigned char USBState = STATE_IDLE;

/// Device LUNs.
MSDLun luns[MAX_LUNS];

/// LUN read/write buffer.
unsigned char msdBuffer[BLOCK_SIZE];

volatile int timeout=2400;
//------------------------------------------------------------------------------
//         Local functions
//------------------------------------------------------------------------------


 //------------------------------------------------------------------------------
 /// Initialize memory for LUN
 //------------------------------------------------------------------------------
 static void MemoryInitialization(void)
 {

     TRACE_DEBUG("LUN SDRAM\n\r");

     LUN_Init(&(luns[0]),0,msdBuffer, 0, 4096000, BLOCK_SIZE);

 }
//------------------------------------------------------------------------------
/// Handler for PIT interrupt. Increments the timestamp counter.
//------------------------------------------------------------------------------
void ISR_Pit(void)
{
    unsigned int status;

    // Read the PIT status register
    status = PIT_GetStatus() & AT91C_PITC_PITS;
    if (status != 0) {

        // Read the PIVR to acknowledge interrupt and get number of ticks
        timestamp += (PIT_GetPIVR() >> 20);
    }
}

//------------------------------------------------------------------------------
/// Configure the periodic interval timer to generate an interrupt every
/// millisecond.
//------------------------------------------------------------------------------
void ConfigurePit(void)
{
    // Initialize the PIT to the desired frequency
    PIT_Init(PIT_PERIOD, BOARD_MCK / 1000000);

    // Configure interrupt on PIT
    AIC_DisableIT(AT91C_ID_SYS);
    AIC_ConfigureIT(AT91C_ID_SYS, AT91C_AIC_PRIOR_LOWEST, ISR_Pit);
    AIC_EnableIT(AT91C_ID_SYS);
    PIT_EnableIT();

    // Enable the pit
    PIT_Enable();
}


//------------------------------------------------------------------------------
//         Callbacks re-implementation
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
/// Invoked when the USB device leaves the Suspended state. By default,
/// configures the LEDs.
//------------------------------------------------------------------------------
void USBDCallbacks_Resumed(void)
{
    USBState = STATE_RESUME;
}

//------------------------------------------------------------------------------
/// Invoked when the USB device gets suspended. By default, turns off all LEDs.
//------------------------------------------------------------------------------
void USBDCallbacks_Suspended(void)
{
    USBState = STATE_SUSPEND;
}

//------------------------------------------------------------------------------
//         Exported functions
//------------------------------------------------------------------------------

#define jump_to_target ((void(*)(void))TARGETSTART)

void checkbootloader() {

  unsigned char volatile * const ram = (unsigned char *) AT91C_ISRAM;
  uint32_t volatile * const fl = (uint32_t *) TARGETSTART;

  AT91C_BASE_PMC->PMC_PCER = (1 << AT91C_ID_PIOA);
  AT91C_BASE_PIOA->PIO_PER = BOOTLOADER_PIN;			//Enable PIO control

  AT91C_BASE_PIOA->PIO_IFER = *ram;

  if(*fl != 0xffffffff) {
    if((AT91C_BASE_PIOA->PIO_PDSR & BOOTLOADER_PIN)) {
      if(*ram != 0xaa) {
        jump_to_target();
      }
    }
    if(*ram == 0x55) {
      jump_to_target();
    }
  }
}

//------------------------------------------------------------------------------
/// Application entry point. Configures the DBGU, PIT, TC0, LEDs and buttons
/// and makes LED\#1 blink in its infinite loop, using the Wait function.
/// \return Unused (ANSI-C compatibility).
//------------------------------------------------------------------------------
int main(void)
{
  unsigned long LEDcounter=0;

  // DBGU configuration
#if (TRACE_LEVEL > TRACE_LEVEL_NO_TRACE)
  TRACE_CONFIGURE(DBGU_STANDARD, 115200, BOARD_MCK);
  puts("\n\rCUBELOADER gestartet\n\r");
#endif
  TRACE_INFO("-- Compiled: %s %s --\n\r", __DATE__, __TIME__);

  //Configure Reset Controller
  AT91C_BASE_RSTC->RSTC_RMR=AT91C_RSTC_URSTEN | 0xa5<<24;

  HAL_GPIO_Init();

  ConfigurePit();

  FLASHD_Initialize(BOARD_MCK);

  led_init();

  //Watchdog off
  AT91C_BASE_WDTC->WDTC_WDMR = AT91C_WDTC_WDDIS;

  //Unlock flash
  unsigned char fl=FLASHD_IsLocked(AT91C_IFLASH + AT91C_IFLASH_LOCK_REGION_SIZE, AT91C_IFLASH + AT91C_IFLASH_SIZE);
  TRACE_INFO("Flash lock %02x\n\r",fl);
  if(fl) {
    FLASHD_Unlock(AT91C_IFLASH + AT91C_IFLASH_LOCK_REGION_SIZE, AT91C_IFLASH + AT91C_IFLASH_SIZE, 0, 0);
    TRACE_INFO("Flash unlocked \n\r");
  }

  uint32_t volatile * const fl1 = (uint32_t *) TARGETSTART;

  AT91C_BASE_PMC->PMC_PCER = (1 << AT91C_ID_PIOA);
  AT91C_BASE_PIOA->PIO_PER = BOOTLOADER_PIN;      //Enable PIO control

  TRACE_INFO("Flash memory %08X:%08X\n\r",TARGETSTART,*fl1);
  TRACE_INFO("RAM memory %08X:%02X\n\r",AT91C_ISRAM,AT91C_BASE_PIOA->PIO_IFSR & 0xff);

  if((AT91C_BASE_PIOA->PIO_PDSR & BOOTLOADER_PIN)) {
    TRACE_INFO("TA2 off\n\r");
  } else {
    TRACE_INFO("TA2 on\n\r");
  }

  TRACE_INFO("Chip ID %08X\n\r",AT91C_BASE_DBGU->DBGU_CIDR );
  TRACE_INFO("Chip ID Extention %08X\n\r",AT91C_BASE_DBGU->DBGU_EXID );

  MemoryInitialization();
  TRACE_INFO("%u Medias defined\n\r", 1);

  // BOT driver initialization
  MSDDriver_Initialize(luns, 1);
  USBD_Connect();


  unsigned char volatile * const ram = (unsigned char *) AT91C_ISRAM;
  *ram = 0xaa;

  // Main loop
  while (1) {

    //CDC_Task();
    MSDDriver_StateMachine();

#ifdef DBGU_UNIT_IN
    if(DBGU_IsRxReady()){
      unsigned char volatile * const ram = (unsigned char *) AT91C_ISRAM;
      unsigned char x;
      unsigned int timeout;

      x=DBGU_GetChar();
      switch(x) {

      case 'd':
        puts("USB disconnect\n\r");
        USBD_Disconnect();
        break;
      case 'c':
        USBD_Connect();
        puts("USB Connect\n\r");
        break;
      case 'S':
        USBD_Disconnect();
        timeout=timestamp;
        while(timestamp<timeout+1000){
          LED_TOGGLE();
        }
        //Reset
        *ram = 0x55;
        AT91C_BASE_RSTC->RSTC_RCR = AT91C_RSTC_PROCRST | AT91C_RSTC_PERRST | AT91C_RSTC_EXTRST   | 0xA5<<24;
        while (1);
        break;
      case 'X':
        USBD_Disconnect();
        timeout=timestamp;
        while(timestamp<timeout+1000){
          LED_TOGGLE();
        }

        //Reset
        *ram = 0x00;
        AT91C_BASE_RSTC->RSTC_RCR = AT91C_RSTC_PROCRST | AT91C_RSTC_PERRST | AT91C_RSTC_EXTRST   | 0xA5<<24;
        while (1);
        break;

      //default:
        //rb_put(&TTY_Tx_Buffer, x);
      }
    }
#endif

    if( USBState == STATE_SUSPEND ) {
      TRACE_INFO("suspend  !\n\r");
      //LowPowerMode();
      USBState = STATE_IDLE;
    }
    if( USBState == STATE_RESUME ) {
      // Return in normal MODE
      TRACE_INFO("resume !\n\r");
      //NormalPowerMode();
      USBState = STATE_IDLE;
    }

    if(timestamp>LEDcounter + 125) {
      LEDcounter=timestamp;
      LED_TOGGLE();
      if(timeout==0) {
        unsigned char volatile * const ram = (unsigned char *) AT91C_ISRAM;
        USBD_Disconnect();
        //Reset
        *ram = 0x55;
        AT91C_BASE_RSTC->RSTC_RCR = AT91C_RSTC_PROCRST | AT91C_RSTC_PERRST | AT91C_RSTC_EXTRST   | 0xA5<<24;
        while (1);
      } else if ( timeout > 0) {
        timeout--;
      }
    }

  }
}

