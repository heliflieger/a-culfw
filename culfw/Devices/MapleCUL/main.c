/* Copyright Telekatz, 2015.
   Released under the GPL Licence, Version 2
*/

//------------------------------------------------------------------------------
//         Headers
//------------------------------------------------------------------------------

#include "board.h"

#include <stm32f1xx_hal.h>
#include <hal_spi.h>
#include <hal_usart.h>
#include <usb_device.h>
#include <hal_gpio.h>
#include <tim.h>
#include <utility/trace.h>

//#include "pio/pio.h"

//#include <dbgu/dbgu.h>
//#include <stdio.h>
//#include <pio/pio_it.h>
//#include <aic/aic.h>
//#include <utility/trace.h>
//#include <usb/device/cdc-serial/CDCDSerialDriver.h>
//#include <usb/device/cdc-serial/CDCDSerialDriverDescriptors.h>
//#include <avr/eeprom.h>
//#include <avr/wdt.h>
//#include <rstc/rstc.h>


#include "led.h"

#include "ringbuffer.h"
#include "ttydata.h"
#include "cdc.h"
#include "clock.h"
#include "fncollection.h"
#include "rf_receive.h"
#include "spi.h"
//#include "fht.h"
#include "rf_send.h"		// fs20send
#include "cc1100.h"
#include "display.h"
//#include "fastrf.h"
//#include "rf_router.h"		// rf_router_func
#include "fband.h"
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

//------------------------------------------------------------------------------
//         Local definitions
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
//         Local variables
//------------------------------------------------------------------------------

/// Global timestamp in milliseconds since start of application.
//volatile unsigned int timestamp = 0;

 /// State of USB, for suspend and resume
 //unsigned char USBState = STATE_IDLE;

 /// Buffer for storing incoming USB data.
 //static unsigned char usbBuffer[DATABUFFERSIZE];

//------------------------------------------------------------------------------
//         Local functions
//------------------------------------------------------------------------------

/** System Clock Configuration
*/
void SystemClock_Config(void)
{

  RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_PeriphCLKInitTypeDef PeriphClkInit;

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }

  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB;
  PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_PLL_DIV1_5;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }

  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);

  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

  /* SysTick_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler */
  /* User can add his own implementation to report the HAL error return state */
  while(1)
  {
  }
  /* USER CODE END Error_Handler */
}

//------------------------------------------------------------------------------
//         Callbacks re-implementation
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/// Callback invoked when data has been received on the USB.
//------------------------------------------------------------------------------
void CDCDSerialDriver_Receive_Callback(uint8_t* Buf, uint32_t *Len)
{
    for(unsigned int i=0;i<*Len;i++) {
      rb_put(&TTY_Rx_Buffer, Buf[i]);
    }
}

//#include "delay.h"


const t_fntab fntab[] = {

  { 'B', prepare_boot },
#ifdef HAS_MBUS
  { 'b', rf_mbus_func },
#endif
  { 'C', ccreg },
//  { 'F', fs20send },
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
//  { 'G', rawsend },
//  { 'M', em_send },
//  { 'K', ks_send },
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
//  { 'R', read_eeprom },
//  { 'T', fhtsend },
  { 'V', version },
//  { 'W', write_eeprom },
  { 'X', set_txreport },

//  { 'e', eeprom_factory_reset },
#ifdef HAS_FASTRF
  { 'f', fastrf_func },
#endif
#ifdef HAS_MEMFN
  { 'm', getfreemem },
#endif
  { 'l', ledfunc },
  { 't', gettime },
#ifdef HAS_RF_ROUTER
  { 'u', rf_router_func },
#endif
//  { 'x', ccsetpa },
#ifdef HAS_ZWAVE
  { 'z', zwave_func },
#endif

  { 0, 0 },
};


//------------------------------------------------------------------------------
//         Exported functions
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/// Application entry point. Configures the DBGU, PIT, TC0, LEDs and buttons
/// and makes LED\#1 blink in its infinite loop, using the Wait function.
/// \return Unused (ANSI-C compatibility).
//------------------------------------------------------------------------------
int main(void)
{


  // DBGU configuration

  TRACE_INFO_WP("\n\r");
  TRACE_INFO("Getting new Started Project --\n\r");
  TRACE_INFO("%s\n\r", BOARD_NAME);
  TRACE_INFO("Compiled: %s %s --\n\r", __DATE__, __TIME__);


  TRACE_INFO("init Flash\n\r");
  //flash_init();

  TRACE_INFO("init Timer\n\r");
  // Configure timer 0
  ticks=0;

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();


  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  MX_USART3_UART_Init();
  MX_USB_DEVICE_Init();
  MX_TIM1_Init();

  led_init();

  TRACE_INFO("init EEprom\n\r");
  //eeprom_init();

  rb_reset(&TTY_Rx_Buffer);
  rb_reset(&TTY_Tx_Buffer);

  input_handle_func = analyze_ttydata;

  LED_OFF();
  LED2_OFF();
  LED3_OFF();

  spi_init();
  //fht_init();
  tx_init();

  #ifdef HAS_ETHERNET
  ethernet_init();
  #endif

  TRACE_INFO("init USB\n\r");
  //CDCDSerialDriver_Initialize();
  //USBD_Connect();

  #ifdef HAS_UART
  uart_init(UART_BAUD_RATE);
  #endif

  //wdt_enable(WDTO_2S);

  //fastrf_on=0;

  display_channel = DISPLAY_USB;

  TRACE_INFO("init Complete\n\r");

  //checkFrequency();

  // Main loop
  while (1) {

    CDC_Task();
    #ifdef HAS_UART
    if(!USB_IsConnected)
      uart_task();
    #endif
    Minute_Task();
    //RfAnalyze_Task();

    #ifdef HAS_FASTRF
      FastRF_Task();
    #endif
    #ifdef HAS_RF_ROUTER
      rf_router_task();
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
    #ifdef HAS_KOPP_FC
      kopp_fc_task();
    #endif
    #ifdef HAS_ZWAVE
      rf_zwave_task();
    #endif
    #ifdef HAS_MAICO
      rf_maico_task();
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




  }
}

