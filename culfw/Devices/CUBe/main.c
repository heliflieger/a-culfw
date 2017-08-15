/* Copyright Telekatz, 2015.
   Released under the GPL Licence, Version 2
   Inpired by the USB CDC Serial Project, Copyright (c) 2008, Atmel Corporation
*/

//------------------------------------------------------------------------------
//         Headers
//------------------------------------------------------------------------------

#include "board.h"
#include "pio/pio.h"
#include "hal_gpio.h"
#include "hal_timer.h"

#include <dbgu/dbgu.h>
#include <stdio.h>
#include <pio/pio_it.h>

#include <utility/trace.h>
#include <usb/device/cdc-serial/CDCDSerialDriver.h>
#include <usb/device/cdc-serial/CDCDSerialDriverDescriptors.h>
#include <avr/eeprom.h>
#include <avr/wdt.h>
#include <rstc/rstc.h>


#include "led.h"

#include "ringbuffer.h"
#include "ttydata.h"
#include "cdc.h"
#include "clock.h"
#include "fncollection.h"
#include "rf_receive.h"
#include "spi.h"
#include "fht.h"
#include "rf_send.h"		// fs20send
#include "cc1100.h"
#include "display.h"
#include "fastrf.h"
#include "rf_router.h"		// rf_router_func
#include "fband.h"
#include "multi_CC.h"
#include "rf_mode.h"
#include "hw_autodetect.h"

#ifdef HAS_UART
#include "serial.h"
#endif

#ifdef HAS_ETHERNET
#include "ethernet.h"
#include "tcplink.h"
#include "ntp.h"
#endif

#ifdef HAS_MEMFN
#include "memory.h"		// getfreemem
#endif
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
#ifdef HAS_ONEWIRE
#include "onewire.h"
#include "i2cmaster.h"
#endif
//------------------------------------------------------------------------------
//         Local definitions
//------------------------------------------------------------------------------

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

//------------------------------------------------------------------------------
//         Local variables
//------------------------------------------------------------------------------

/// Global timestamp in milliseconds since start of application.
volatile unsigned int timestamp = 0;

 /// State of USB, for suspend and resume
 unsigned char USBState = STATE_IDLE;

 /// Buffer for storing incoming USB data.
 static unsigned char usbBuffer[DATABUFFERSIZE];

//------------------------------------------------------------------------------
//         Local functions
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
//         Callbacks re-implementation
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
/// Invoked when the USB device leaves the Suspended state. By default,
/// configures the LEDs.
//------------------------------------------------------------------------------
void USBDCallbacks_Resumed(void)
{
  LED3_ON();
  USBState = STATE_RESUME;
}

//------------------------------------------------------------------------------
/// Invoked when the USB device gets suspended. By default, turns off all LEDs.
//------------------------------------------------------------------------------
void USBDCallbacks_Suspended(void)
{
  LED3_OFF();
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

        TRACE_WARNING("UsbDataReceived: %u bytes discarded\n\r",remaining);
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

#include "delay.h"


const t_fntab fntab[] = {

  { 'B', prepare_boot },
#ifdef HAS_MBUS
  { 'b', rf_mbus_func },
#endif
  { 'C', ccreg },
  { 'F', fs20send },
#ifdef HAS_INTERTECHNO
  { 'i', it_func },
#endif
#ifdef HAS_ASKSIN
  { 'A', asksin_func },
#endif
#ifdef HAS_MORITZ
  { 'Z', moritz_func },
#endif
#ifdef HAS_RFNATIVE
  { 'N', native_func },
#endif
#ifdef HAS_RWE
  { 'E', rwe_func },
#endif
#ifdef HAS_KOPP_FC
  { 'k', kopp_fc_func },
#endif
#ifdef HAS_RAWSEND
  { 'G', rawsend },
  { 'M', em_send },
  { 'K', ks_send },
#endif
#ifdef HAS_MAICO
  { 'L', maico_func },
#endif
#ifdef HAS_UNIROLL
  { 'U', ur_send },
#endif
#ifdef HAS_SOMFY_RTS
  { 'Y', somfy_rts_func },
#endif
  { 'R', read_eeprom },
  { 'T', fhtsend },
  { 'V', version },
  { 'W', write_eeprom },
  { 'X', set_txreport },
#ifdef HAS_ONEWIRE
  { 'O', onewire_func },
#endif
  { 'e', eeprom_factory_reset },
#ifdef HAS_FASTRF
  { 'f', fastrf_func },
#endif
#ifdef HAS_HOERMANN_SEND
  { 'h', hm_send },
#endif
#ifdef HAS_MEMFN
  { 'm', getfreemem },
#endif
  { 'l', ledfunc },
  { 't', gettime },
#ifdef HAS_RF_ROUTER
  { 'u', rf_router_func },
#endif
  { 'x', ccsetpa },
#ifdef HAS_ZWAVE
  { 'z', zwave_func },
#endif
#if HAS_MULTI_CC > 1
  { '*', multiCC_func },
#endif
  { 0, 0 },
};

#if HAS_MULTI_CC > 1
const t_fntab fntab1[] = {

#ifdef HAS_MBUS
  { 'b', rf_mbus_func },
#endif
  { 'C', ccreg },
  { 'F', fs20send },
#ifdef HAS_INTERTECHNO
  { 'i', it_func },
#endif
#ifdef HAS_ASKSIN
  { 'A', asksin_func },
#endif
#ifdef HAS_MORITZ
  { 'Z', moritz_func },
#endif
#ifdef HAS_RFNATIVE
  { 'N', native_func },
#endif
#ifdef HAS_RWE
  { 'E', rwe_func },
#endif
  { 'G', rawsend },
  { 'M', em_send },
  { 'K', ks_send },
#ifdef HAS_MAICO
  { 'L', maico_func },
#endif
#ifdef HAS_UNIROLL
  { 'U', ur_send },
#endif
#ifdef HAS_SOMFY_RTS
  { 'Y', somfy_rts_func },
#endif
  { 'R', read_eeprom },
  { 'T', fhtsend },
  { 'V', version },
  { 'W', write_eeprom },
  { 'X', set_txreport },
#ifdef HAS_FASTRF
  { 'f', fastrf_func },
#endif
#ifdef HAS_ZWAVE
  { 'z', zwave_func },
#endif
#if HAS_MULTI_CC > 2
  { '*', multiCC_func },
#endif
  { 0, 0 },
};
#endif

#if HAS_MULTI_CC > 2
const t_fntab fntab2[] = {

#ifdef HAS_MBUS
  { 'b', rf_mbus_func },
#endif
  { 'C', ccreg },
#ifdef HAS_ASKSIN
  { 'A', asksin_func },
#endif
#ifdef HAS_MORITZ
  { 'Z', moritz_func },
#endif
#ifdef HAS_RFNATIVE
  { 'N', native_func },
#endif
#ifdef HAS_RWE
  { 'E', rwe_func },
#endif
#ifdef HAS_MAICO
  { 'L', maico_func },
#endif
#ifdef HAS_SOMFY_RTS
  { 'Y', somfy_rts_func },
#endif
  { 'V', version },
  { 'X', set_txreport },
#ifdef HAS_FASTRF
  { 'f', fastrf_func },
#endif
#ifdef HAS_ZWAVE
  { 'z', zwave_func },
#endif
#if HAS_MULTI_CC > 3
  { '*', multiCC_func },
#endif
  { 0, 0 },
};
#endif

#if HAS_MULTI_CC > 3
const t_fntab fntab3[] = {

#ifdef HAS_MBUS
  { 'b', rf_mbus_func },
#endif
  { 'C', ccreg },
#ifdef HAS_ASKSIN
  { 'A', asksin_func },
#endif
#ifdef HAS_MORITZ
  { 'Z', moritz_func },
#endif
#ifdef HAS_RFNATIVE
  { 'N', native_func },
#endif
#ifdef HAS_RWE
  { 'E', rwe_func },
#endif
#ifdef HAS_MAICO
  { 'L', maico_func },
#endif
#ifdef HAS_SOMFY_RTS
  { 'Y', somfy_rts_func },
#endif
  { 'V', version },
  { 'X', set_txreport },
#ifdef HAS_FASTRF
  { 'f', fastrf_func },
#endif
#ifdef HAS_ZWAVE
  { 'z', zwave_func },
#endif
    { 0, 0 },
};
#endif

//------------------------------------------------------------------------------
//         Exported functions
//------------------------------------------------------------------------------

//-----------------------------------------------------------------------------
/// Global function for uIP to use
/// \param m Pointer to string that logged
//-----------------------------------------------------------------------------
void uip_log(char *m)
{
  TRACE_INFO_WP("-uIP- %s\n\r", m);
}

static const Pin emacRstPins[] = {BOARD_EMAC_RST_PINS};

//------------------------------------------------------------------------------
/// Application entry point. Configures the DBGU, PIT, TC0, LEDs and buttons
/// and makes LED\#1 blink in its infinite loop, using the Wait function.
/// \return Unused (ANSI-C compatibility).
//------------------------------------------------------------------------------
int main(void)
{


  // DBGU configuration
  TRACE_CONFIGURE(DBGU_STANDARD, 115200, BOARD_MCK);
  TRACE_INFO_WP("\n\r");
  TRACE_INFO("Getting new Started Project --\n\r");
  TRACE_INFO("%s\n\r", BOARD_NAME);
  TRACE_INFO("Compiled: %s %s --\n\r", __DATE__, __TIME__);

  //Configure Reset Controller
  AT91C_BASE_RSTC->RSTC_RMR= 0xa5<<24;

  // Configure EMAC PINS
  PIO_Configure(emacRstPins, PIO_LISTSIZE(emacRstPins));

  // Execute reset
  RSTC_SetExtResetLength(0xd);
  RSTC_ExtReset();

  // Wait for end hardware reset
  while (!RSTC_GetNrstLevel());

  HAL_GPIO_Init();

  TRACE_INFO("init Flash\n\r");
  flash_init();

  TRACE_INFO("init Timer\n\r");
  // Configure timer 0
  ticks=0;

  HAL_timer_init();

  led_init();

  TRACE_INFO("init EEprom\n\r");
  eeprom_init();

  rb_reset(&TTY_Rx_Buffer);
  rb_reset(&TTY_Tx_Buffer);

  input_handle_func = analyze_ttydata;

  LED_OFF();
  LED2_OFF();
  LED3_OFF();

  spi_init();
  fht_init();

  #ifdef HAS_ETHERNET
  ethernet_init();
  #endif

  #ifdef USE_HW_AUTODETECT
  hw_autodetect();
  #endif

  tx_init();

  #ifdef HAS_ONEWIRE
  #ifdef USE_HW_AUTODETECT
  if(has_onewire())
  #endif
  {
    TRACE_INFO("init ONEWIRE\n\r");
    i2c_init();
    onewire_Init();
    onewire_FullSearch();
  }
  #endif

  TRACE_INFO("init USB\n\r");
  CDCDSerialDriver_Initialize();
  USBD_Connect();

  #ifdef HAS_UART
  uart_init(UART_BAUD_RATE);
  #endif

  wdt_enable(WDTO_2S);

  fastrf_on=0;

  display_channel = DISPLAY_USB;

  TRACE_INFO("init Complete\n\r");

#if defined(HAS_MULTI_CC)
  for (CC1101.instance = 0; CC1101.instance < HAS_MULTI_CC; CC1101.instance++)
    checkFrequency();
  CC1101.instance = 0;
#else
  checkFrequency();
#endif

  // Main loop
  while (1) {

    CDC_Task();
    #ifdef HAS_UART
    if(!USB_IsConnected)
      uart_task();
    #endif
    Minute_Task();

#ifdef USE_RF_MODE
    rf_mode_task();
#else
    RfAnalyze_Task();
    #ifdef HAS_FASTRF
      FastRF_Task();
    #endif
    #ifdef HAS_ASKSIN
      rf_asksin_task();
    #endif
    #ifdef HAS_MORITZ
      rf_moritz_task();
    #endif
    #ifdef HAS_RWE
      rf_rwe_task();
    #endif
    #ifdef HAS_MBUS
      rf_mbus_task();
    #endif
    #ifdef HAS_RFNATIVE
      native_task();
    #endif
    #ifdef HAS_ZWAVE
      rf_zwave_task();
    #endif
    #ifdef HAS_MAICO
      rf_maico_task();
    #endif
#endif

    #ifdef HAS_RF_ROUTER
      rf_router_task();
    #endif
    #ifdef HAS_KOPP_FC
      kopp_fc_task();
    #endif
    #ifdef HAS_ETHERNET
      Ethernet_Task();
    #endif

#ifdef DBGU_UNIT_IN
    if(DBGU_IsRxReady()){
      unsigned char volatile * const ram = (unsigned char *) AT91C_ISRAM;
      unsigned char x;

      x=DBGU_GetChar();
      switch(x) {

      case 'd':
        puts("USB disconnect\n\r");
        USBD_Disconnect();
        LED3_OFF();
        break;
      case 'c':
        USBD_Connect();
        puts("USB Connect\n\r");
        break;
      case 'r':
        //Configure Reset Controller
        AT91C_BASE_RSTC->RSTC_RMR=AT91C_RSTC_URSTEN | 0xa5<<24;
        break;
      case 'H':
#ifdef USE_HW_AUTODETECT
  hw_autodetect();
#endif

        break;
      case 'S':
        USBD_Disconnect();

        my_delay_ms(250);
        my_delay_ms(250);

        //Reset
        *ram = 0xaa;
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
        LED3_ON();
        USBState=STATE_RX;
      }
    }
    if( USBState == STATE_SUSPEND ) {
      TRACE_INFO("suspend  !\n\r");
      USBState = STATE_IDLE;
    }
    if( USBState == STATE_RESUME ) {
      TRACE_INFO("resume !\n\r");
      USBState = STATE_IDLE;
    }

  }
}

