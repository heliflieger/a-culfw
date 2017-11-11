/* Includes ------------------------------------------------------------------*/
#include "board.h"
#include "hal_usart.h"
#include <stm32f103xb.h>

uint8_t DBGU_RxByte;
uint8_t DBGU_RxReady;

void SWO_PrintChar(char c, uint8_t portNo) {
  volatile int timeout;

    /* Check if Trace Control Register (ITM->TCR at 0xE0000E80) is set */
    if ((ITM->TCR&ITM_TCR_ITMENA_Msk) == 0) { /* check Trace Control Register if ITM trace is enabled*/
      return; /* not enabled? */
    }
    /* Check if the requested channel stimulus port (ITM->TER at 0xE0000E00) is enabled */
    if ((ITM->TER & (1ul<<portNo))==0) { /* check Trace Enable Register if requested port is enabled */
      return; /* requested port not enabled? */
    }
    timeout = 5000; /* arbitrary timeout value */
    while (ITM->PORT[portNo].u32 == 0) {
      /* Wait until STIMx is ready, then send data */
      timeout--;
      if (timeout==0) {
        return; /* not able to send */
      }
    }
    ITM->PORT[portNo].u8 = (uint8_t)c;
    //ITM->PORT[0].u16 = 0x08 | (c<<8);
}

int fputc(int ch, FILE *f) {
#ifndef HAS_UART
  HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, 0xFFFF);
#endif
  SWO_PrintChar((uint8_t)ch,0);
  return ch;
}

signed int fputs(const char *pStr, FILE *pStream)
{
    signed int num = 0;

    while (*pStr != 0) {

        if (fputc(*pStr, pStream) == -1) {

            return -1;
        }
        num++;
        pStr++;
    }

    return num;
}

void DBGU_init(void) {
#ifndef HAS_UART
  MX_USART1_UART_Init();
#endif
}

unsigned int DBGU_IsRxReady() {
    return DBGU_RxReady;
}


unsigned char DBGU_GetChar(void) {
  DBGU_RxReady = 0;
  return DBGU_RxByte;
}

/*
uint32_t _ITMPort = 0; // The stimulus port from which SWO data is received and displayed.
uint32_t TargetDiv = 12;// Has to be calculated according to the CPU speed and the output baud rate


void SWO_Init(uint32_t portBits, uint32_t cpuCoreFreqHz) {

  uint32_t StimulusRegs;
  //
  // Enable access to SWO registers
  //
  CoreDebug->DEMCR |= (1 << 24);
  ITM->LAR = 0xC5ACCE55;
  //
  // Initially disable ITM and stimulus port
  // To make sure that nothing is transferred via SWO
  // when changing the SWO prescaler etc.
  //
  StimulusRegs = ITM->TER;
  StimulusRegs &= ~(1 << _ITMPort);
  ITM->TER = StimulusRegs; // Disable ITM stimulus port
  ITM->TCR = 0; // Disable ITM
  //
  // Initialize SWO (prescaler, etc.)
  //
  TPI->SPPR = 0x00000002; // Select NRZ mode
  TPI->ACPR = TargetDiv - 1; // Example: 72/48 = 1,5 MHz
  ITM->TPR = 0x00000000;
  DWT->CTRL = 0x400003FF;
  TPI->FFCR = 0x00000100;
  //
  // Enable ITM and stimulus port
  //
  ITM->TCR = 0x1000D; // Enable ITM
  ITM->TER = StimulusRegs | (1 << _ITMPort); // Enable ITM stimulus port

}
*/

