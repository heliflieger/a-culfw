/* Copyright Telekatz, 2016.
   Released under the GPL Licence, Version 2
*/

//------------------------------------------------------------------------------
//         Headers
//------------------------------------------------------------------------------

#include <stm32f1xx_hal.h>
#include <stm32f103xb.h>
#include <hal_spi.h>
#include <hal_usart.h>
#include <usb_device.h>
#include <hal_gpio.h>
#include <hal_timer.h>
#include <utility/trace.h>
#include <utility/dbgu.h>
#include <avr/eeprom.h>
#include <avr/wdt.h>

#include "led.h"

#include "ringbuffer.h"
#include "ttydata.h"
#include "cdc.h"
#include "clock.h"
#include "delay.h"
#include "fncollection.h"
#include "rf_receive.h"
#include "spi.h"
#include "fht.h"
#include "rf_send.h"
#include "cc1100.h"
#include "display.h"
#include "fastrf.h"
#include "fband.h"
#include "multi_CC.h"
#include "rf_mode.h"
#include "hw_autodetect.h"

#include "board.h"
#ifdef HAS_UART
#include "serial.h"
#endif
#ifdef HAS_WIZNET
#include "ethernet.h"
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
//-----

#include "cdc_uart.h"
//------------------------------------------------------------------------------
//         Local definitions
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
//         Local variables
//------------------------------------------------------------------------------


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


  /* Enable TRC */
  CoreDebug->DEMCR &= ~0x01000000;
  CoreDebug->DEMCR |=  0x01000000;

  /* Enable counter */
  DWT->CTRL &= ~0x00000001;
  DWT->CTRL |=  0x00000001;

  /* Reset counter */
  DWT->CYCCNT = 0;

}

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
void Error_Handler(void)
{
  while(1)
  {
  }
}

//------------------------------------------------------------------------------
//         Callbacks re-implementation
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Callback invoked when data has been received on the USB.
//------------------------------------------------------------------------------
uint8_t CDCDSerialDriver_Receive_Callback0(uint8_t* Buf, uint32_t *Len)
{
    for(unsigned int i=0;i<*Len;i++) {
      rb_put(&TTY_Rx_Buffer, Buf[i]);
    }
    return 1;
}

//------------------------------------------------------------------------------
//         Function Table
//------------------------------------------------------------------------------

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
#ifdef HAS_MEMFN
  { 'm', getfreemem },
#endif
  { 'l', ledfunc },
#if CDC_COUNT > 1
  { 'p', cdc_uart_func },
#endif
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
#if NUM_SLOWRF > 1
  { 'W', write_eeprom },
#endif
  { 'X', set_txreport },
#ifdef HAS_FASTRF
  { 'f', fastrf_func },
#endif
  { 'x', ccsetpa },
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
#if NUM_SLOWRF > 2
  { 'W', write_eeprom },
#endif
  { 'X', set_txreport },
#ifdef HAS_FASTRF
  { 'f', fastrf_func },
#endif
  { 'x', ccsetpa },
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


//------------------------------------------------------------------------------
/// Application entry point.
/// \return Unused (ANSI-C compatibility).
//------------------------------------------------------------------------------
int main(void)
{
  HAL_Init();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM1_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_TIM4_Init();

  DBGU_init();

  /* Configure the system clock */
  SystemClock_Config();

  TRACE_INFO_WP("\n\r");
  TRACE_INFO("Getting new Started Project --\n\r");
  TRACE_INFO("%s\n\r", BOARD_NAME);
  TRACE_INFO("Compiled: %s %s --\n\r", __DATE__, __TIME__);

  TRACE_INFO("init Flash\n\r");
  flash_init();

  //Configure timer
  TRACE_INFO("init Timer\n\r");
  ticks=0;

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

  #ifdef HAS_WIZNET
  TRACE_INFO("init Ethernet\n\r");
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
  MX_USB_DEVICE_Init();

  #ifdef HAS_UART
  uart_init(UART_BAUD_RATE);
  #endif

  #if CDC_COUNT > 1
  cdc_uart_init();
  #endif

  wdt_enable(WDTO_2S);

  fastrf_on=0;

  display_channel = DISPLAY_USB;

#ifdef HAS_WIZNET
#ifdef USE_HW_AUTODETECT
  if(has_ethernet())
#endif
    display_channel |= DISPLAY_TCP;
#endif

  USBD_Disconnect();
  my_delay_ms(15);
  USBD_Connect();

#if defined(HAS_MULTI_CC)
  for (CC1101.instance = 0; CC1101.instance < HAS_MULTI_CC; CC1101.instance++)
    checkFrequency();
  CC1101.instance = 0;
#else
  checkFrequency();
#endif

  TRACE_INFO("init Complete\n\r");

  // Main loop
  while (1) {

#ifdef HAS_UART
    if(!USB_IsConnected)
      uart_task();
#endif

    CDC_Task();
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
    #if CDC_COUNT > 1
      cdc_uart_task();
    #endif


    #ifdef HAS_WIZNET
    #ifdef USE_HW_AUTODETECT
    if(has_ethernet())
    #endif
      Ethernet_Task();
    #endif

#ifdef DBGU_UNIT_IN
    if(DBGU_IsRxReady()){
      unsigned char x;

      x=DBGU_GetChar();
      SWO_PrintChar(x,0);
      switch(x) {

      case 'd':
        puts("USB disconnect\n\r");
        USBD_Disconnect();
        break;
      case 'c':
        USBD_Connect();
        puts("USB Connect\n\r");
        break;
      case 'r':

        break;
      case 'w':

        break;
      default:
        rb_put(&TTY_Tx_Buffer, x);
      }
    }
#endif




  }
}

