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
#include <usb/device/cdc-serial/CDCDSerialDriver.h>
#include <usb/device/cdc-serial/CDCDSerialDriverDescriptors.h>
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
#define VERSION 	"1.01"

/// PIT period value in µseconds.
#define PIT_PERIOD          1000

/// Size in bytes of the buffer used for reading data from the USB
#define DATABUFFERSIZE \
    BOARD_USB_ENDPOINTS_MAXPACKETSIZE(CDCDSerialDriverDescriptors_DATAIN)

/// Use for power management
#define STATE_IDLE    0
/// The USB device is in suspend state
#define STATE_SUSPEND 4
/// The USB device is in resume state
#define STATE_RESUME  5
/// The USB device is in resume state
#define STATE_RX	  6
/// Start programm
#define TARGETSTART		0x104000
/// Xmodem
#define SOH				0x01
#define NAK				0x15
#define ACK				0x06
#define EOT				0x04
#define XPACKET			0x01
#define XDATA			0x03
#define XCHKSUM			0x83
#define XDATALEN		0x80
#define CMDBUFFERLEN	132
#define WRITE_PACKETS	8
#define WRITEBUFFERLEN	(XDATALEN * WRITE_PACKETS)

//------------------------------------------------------------------------------
//         Local variables
//------------------------------------------------------------------------------

///Xmodem
unsigned char xmodem=0;
unsigned long lasttimestamp=0;
unsigned char nak_timeout=0;
unsigned char packetcounter=0;
unsigned long packetcounterL=0;
unsigned long cmdbuffercounter=0;
unsigned char cmdbuffer[CMDBUFFERLEN];
unsigned char packetscount=0;
unsigned char writebuffer[WRITEBUFFERLEN];
unsigned int writeaddress=TARGETSTART;

/// Global timestamp in milliseconds since start of application.
volatile unsigned int timestamp = 0;

 /// State of USB, for suspend and resume
 unsigned char USBState = STATE_IDLE;

 /// Buffer for storing incoming USB data.
 static unsigned char usbBuffer[DATABUFFERSIZE];

#define DATABUFFERSIZEOUT \
    BOARD_USB_ENDPOINTS_MAXPACKETSIZE(CDCDSerialDriverDescriptors_DATAOUT)

 static unsigned char usbBufferOut[DATABUFFERSIZEOUT];

 rb_t TTY_Tx_Buffer;
 rb_t TTY_Rx_Buffer;

//------------------------------------------------------------------------------
//         Local functions
//------------------------------------------------------------------------------


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
/// Callback invoked when data has been received on the USB.
//------------------------------------------------------------------------------
static void UsbDataReceived(unsigned int unused,
                            unsigned char status,
                            unsigned int received,
                            unsigned int remaining)
{
    // Check that data has been received successfully
    if (status == USBD_STATUS_SUCCESS) {

    	for(unsigned int i=0;i<received;i++) {
    		rb_put(&TTY_Rx_Buffer, usbBuffer[i]);
    	}

        // Check if bytes have been discarded
        if ((received == DATABUFFERSIZE) && (remaining > 0)) {

            TRACE_WARNING(
                      "UsbDataReceived: %u bytes discarded\n\r",
                      remaining);
        }
    }
    else {

        TRACE_WARNING( "UsbDataReceived: Transfer error\n\r");
    }

    // Restart USB read
   CDCDSerialDriver_Read(usbBuffer,
                         DATABUFFERSIZE,
                         (TransferCallback) UsbDataReceived,
                         0);

}

signed int puts2(const char *pStr)
{
    signed int num = 0;

    while (*pStr != 0) {

    	rb_put(&TTY_Tx_Buffer, *pStr);
        num++;
        pStr++;
    }

    return num;
}


////////////////////
// Fill data from USB to the RingBuffer and vice-versa
void
CDC_Task(void)
{

  unsigned char x;

  if(!USB_IsConnected)
    return;

  switch(xmodem) {
  case 0:
    while(TTY_Rx_Buffer.nbytes && !xmodem) {
      x=rb_get(&TTY_Rx_Buffer);
      if(x==SOH) {
        //Beginn Übertragung
        xmodem=2;
        lasttimestamp=timestamp;
        nak_timeout=0;
        packetcounter=0;
        packetcounterL=0;
        writeaddress=TARGETSTART;
        cmdbuffercounter=0;
        cmdbuffer[cmdbuffercounter++]=x;
        lasttimestamp=timestamp;
        TRACE_INFO("-- Xmodem start\n\r");
      } else {
        switch(x) {
        case 'V':
          puts2("\n\rCUBELOADER V" VERSION "\n\r");
          break;
        default:
          DBGU_PutChar(x);
        }
      }
    }
    if(xmodem == 0) {
      //Alle 5s NAK senden
      if((timestamp > lasttimestamp+5000) && !packetcounterL) {
        rb_put(&TTY_Tx_Buffer, NAK);
        lasttimestamp=timestamp;
      }
    }
    break;
  case 1:

    if(nak_timeout > 10) {
      //Nach 10 NAK abbrechen
      xmodem=0;
      TRACE_INFO("-- Xmodem abort; nak_timeout\n\r");
    }

    if(TTY_Rx_Buffer.nbytes) {
      x=rb_get(&TTY_Rx_Buffer);
      if(x==SOH) {
        xmodem=2;
        cmdbuffercounter=0;
        cmdbuffer[cmdbuffercounter++]=x;
        lasttimestamp=timestamp;
        TRACE_INFO("-- Xmodem SOH\n\r");
        nak_timeout=0;
      } else if (x==EOT) {
        xmodem=3;
        TRACE_INFO("-- Xmodem EOT; send ACK\n\r");
        rb_put(&TTY_Tx_Buffer, ACK);
        if(packetscount) {
          TRACE_INFO("-- Xmodemwrite flash; %08X:\n\r",writeaddress);
          FLASHD_Write(writeaddress,writebuffer,packetscount * XDATALEN);
          writeaddress+=packetscount * XDATALEN;
          packetscount=0;
        }
      } else {

      }
    } else {
      if(timestamp > lasttimestamp+5000) {
        //Nach 5s inaktivität abbrechen
        xmodem=0;
        TRACE_INFO("-- Xmodem abort; timeout\n\r");
      }
    }
    break;
  case 2:
    while((TTY_Rx_Buffer.nbytes) && (xmodem==2)) {
      cmdbuffer[cmdbuffercounter++]=rb_get(&TTY_Rx_Buffer);

      if(cmdbuffercounter==CMDBUFFERLEN) {
        unsigned char checksum=0;
        for(unsigned long i=0;i<XDATALEN;i++) {
          checksum+=cmdbuffer[i+XDATA];
        }
        if(cmdbuffer[XPACKET]==packetcounter) {
          //Paket bereits empfangen
          rb_put(&TTY_Tx_Buffer, ACK);
          TRACE_INFO("-- Xmodem packet %02u received again; send ACK:\n\r",cmdbuffer[XPACKET]);
        } else if (cmdbuffer[XPACKET]!=(unsigned char)(packetcounter+1)) {
          //Falsche Paketnummer
          nak_timeout++;
          rb_put(&TTY_Tx_Buffer, NAK);
          TRACE_INFO("-- Xmodem wrong packet %02u received; send NAK:\n\r",cmdbuffer[XPACKET]);
        } else if (cmdbuffer[XCHKSUM]!=checksum) {
          //Falsche Paketnummer
          nak_timeout++;
          rb_put(&TTY_Tx_Buffer, NAK);
          TRACE_INFO("-- Xmodem wrong checksum; send NAK:\n\r");
        } else {
          //Paket OK
          TRACE_INFO("-- Xmodem packet %02u received; send ACK\n\r",cmdbuffer[XPACKET]);
          for(unsigned long i=0; i<XDATALEN;i++) {
            writebuffer[i+(XDATALEN * packetscount)]=cmdbuffer[i+XDATA];
          }
          packetscount++;

          if(packetscount==WRITE_PACKETS) {

            TRACE_INFO("-- Xmodemwrite flash; %08X:\n\r",writeaddress);
            FLASHD_Write(writeaddress,writebuffer,WRITEBUFFERLEN);
            writeaddress+=WRITEBUFFERLEN;
            packetscount=0;
          }
          nak_timeout=0;
          packetcounter++;
          packetcounterL++;
          cmdbuffercounter=0;
          rb_put(&TTY_Tx_Buffer, ACK);

        }
        xmodem=1;
      }
      lasttimestamp=timestamp;
    }
    if(timestamp > lasttimestamp+5000) {
      //Nach 5s inaktivität NAK senden
      nak_timeout++;
      lasttimestamp=timestamp;
      rb_put(&TTY_Tx_Buffer, NAK);
      TRACE_INFO("-- Xmodem receive timeout; send NAK\n\r");
      xmodem=1;
    }
    break;
  case 3:
    //Reset
    TRACE_INFO("Restart\n\r");
    {
      unsigned char volatile * const ram = (unsigned char *) AT91C_ISRAM;
      unsigned int timeout;

      USBD_Disconnect();
      timeout=timestamp;
      while(timestamp<timeout+1000){
        LED_TOGGLE();
      }

      *ram = 0x55;
      AT91C_BASE_RSTC->RSTC_RCR = AT91C_RSTC_PROCRST | AT91C_RSTC_PERRST | AT91C_RSTC_EXTRST   | 0xA5<<24;
      while (1);
    }
    xmodem=0;
  }


  if(TTY_Tx_Buffer.nbytes) {
    unsigned long i=0;

    while(TTY_Tx_Buffer.nbytes && i<DATABUFFERSIZEOUT) {

       usbBufferOut[i++]=rb_get(&TTY_Tx_Buffer);
    }

    while (CDCDSerialDriver_Write(usbBufferOut,i, 0, 0) != USBD_STATUS_SUCCESS);

  }


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
  TRACE_CONFIGURE(DBGU_STANDARD, 115200, BOARD_MCK);
  puts("\n\rCUBELOADER gestartet\n\r");
  TRACE_INFO("-- Compiled: %s %s --\n\r", __DATE__, __TIME__);

  //Configure Reset Controller
  AT91C_BASE_RSTC->RSTC_RMR=AT91C_RSTC_URSTEN | 0xa5<<24;

  HAL_GPIO_Init();

  ConfigurePit();

  FLASHD_Initialize(BOARD_MCK);

  rb_reset(&TTY_Rx_Buffer);
  rb_reset(&TTY_Tx_Buffer);

  TRACE_INFO("-- init USB\n\r");
  CDCDSerialDriver_Initialize();
  USBD_Connect();

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

    // Main loop
    while (1) {

      CDC_Task();

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

        default:
          rb_put(&TTY_Tx_Buffer, x);
        }
      }
#endif

    if (USBD_GetState() == USBD_STATE_CONFIGURED) {
      if( USBState == STATE_IDLE ) {
        CDCDSerialDriver_Read(usbBuffer,
                              DATABUFFERSIZE,
                              (TransferCallback) UsbDataReceived,
                              0);
        USBState=STATE_RX;
      }
    }
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
    }
  }
}

