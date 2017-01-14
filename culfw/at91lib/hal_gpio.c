/* Includes ------------------------------------------------------------------*/
#include "hal_gpio.h"
#include <board.h>
#include "led.h"
#include "delay.h"
#include <pio/pio.h>
#include <aic/aic.h>

static const Pin pinsLeds[] = {PINS_LEDS};

const transceiver_t CCtransceiver[] = CCTRANSCEIVERS;


/*----------------------------------------------------------------------------*/
/* USB                                                                        */
/*----------------------------------------------------------------------------*/

void HAL_GPIO_Init(void) {
  AT91C_BASE_PMC->PMC_PCER = (1 << AT91C_ID_PIOA);
  #ifdef CUBE
  AT91C_BASE_PMC->PMC_PCER = (1 << AT91C_ID_PIOB);
  #endif
}

void hal_UCBD_connect_init(void) {
#ifdef USBD_CONNECT_PIN

#endif
}


/*----------------------------------------------------------------------------*/
/* LED                                                                        */
/*----------------------------------------------------------------------------*/
void HAL_LED_Init(void) {
  for(uint8_t x=0; x<LED_COUNT; x++) {
    PIO_Configure(&pinsLeds[x], 1);
  }
}

void HAL_LED_Set(LED_List led, LED_State state) {
  if(led < LED_COUNT) {
    if(state)
      PIO_Clear(&pinsLeds[led]);
    else
      PIO_Set(&pinsLeds[led]);
  }
}

void HAL_LED_Toggle(uint8_t led) {
  if(led < LED_COUNT) {
    if(PIO_GetOutputDataStatus(&pinsLeds[led]))
      PIO_Clear(&pinsLeds[led]);
    else
      PIO_Set(&pinsLeds[led]);
  }
}


/*----------------------------------------------------------------------------*/
/* CC1101                                                                     */
/*----------------------------------------------------------------------------*/
void hal_CC_GDO_init(uint8_t cc_num, uint8_t mode) {
  /*Configure GDO0 (out) pin */
  if(mode == INIT_MODE_IN_CS_IN) {
    CCtransceiver[cc_num].base[CC_Pin_Out]->PIO_ODR = _BV(CCtransceiver[cc_num].pin[CC_Pin_Out]);   //Enable input
    CCtransceiver[cc_num].base[CC_Pin_Out]->PIO_PER = _BV(CCtransceiver[cc_num].pin[CC_Pin_Out]);     //Enable PIO control
  } else {
    CCtransceiver[cc_num].base[CC_Pin_Out]->PIO_PPUER = _BV(CCtransceiver[cc_num].pin[CC_Pin_Out]);   //Enable pullup
    CCtransceiver[cc_num].base[CC_Pin_Out]->PIO_OER   = _BV(CCtransceiver[cc_num].pin[CC_Pin_Out]);   //Enable output
    CCtransceiver[cc_num].base[CC_Pin_Out]->PIO_PER   = _BV(CCtransceiver[cc_num].pin[CC_Pin_Out]);   //Enable PIO control
  }

  /*Configure GDO1 (CS) pin */
  CCtransceiver[cc_num].base[CC_Pin_CS]->PIO_PPUER = _BV(CCtransceiver[cc_num].pin[CC_Pin_CS]);   //Enable pullup
  CCtransceiver[cc_num].base[CC_Pin_CS]->PIO_OER   = _BV(CCtransceiver[cc_num].pin[CC_Pin_CS]);   //Enable output
  CCtransceiver[cc_num].base[CC_Pin_CS]->PIO_PER   = _BV(CCtransceiver[cc_num].pin[CC_Pin_CS]);   //Enable PIO control

  /*Configure GDO2 (in) pin */
  CCtransceiver[cc_num].base[CC_Pin_In]->PIO_IER = _BV(CCtransceiver[cc_num].pin[CC_Pin_In]);     //Enable input change interrupt
  CCtransceiver[cc_num].base[CC_Pin_In]->PIO_ODR = _BV(CCtransceiver[cc_num].pin[CC_Pin_In]);     //Enable input
  CCtransceiver[cc_num].base[CC_Pin_In]->PIO_PER = _BV(CCtransceiver[cc_num].pin[CC_Pin_In]);     //Enable PIO control

}

__attribute__((weak)) void CC1100_in_callback()
{
  /* NOTE : This function Should not be modified, when the callback is needed,
            the callback could be implemented in the user file
   */
}

void ISR_PioA() {
  // Read PIO controller status
  AT91C_BASE_PIOA->PIO_ISR;
  CC1100_in_callback();

}

#ifdef AT91C_ID_PIOB
void ISR_PioB() {
  // Read PIO controller status
  AT91C_BASE_PIOB->PIO_ISR;
  CC1100_in_callback();

}
#endif

void hal_enable_CC_GDOin_int(uint8_t cc_num, uint8_t enable) {
  /*Configure in pin */
  uint32_t PIO_ID;
  void* ISR_Pio;

  if(cc_num)
    return;

#ifdef AT91C_ID_PIOB
  if(CCtransceiver[cc_num].base[CC_Pin_In] == AT91C_BASE_PIOA) {
    PIO_ID = AT91C_ID_PIOA;
    ISR_Pio = ISR_PioA;
  } else {
    PIO_ID = AT91C_ID_PIOB;
    ISR_Pio = ISR_PioB;
  }
#else
  PIO_ID = AT91C_ID_PIOA;
  ISR_Pio = ISR_PioA;
#endif

  if(enable) {
    AIC_ConfigureIT(PIO_ID, AT91C_AIC_PRIOR_HIGHEST, ISR_Pio);
    AT91C_BASE_AIC->AIC_IECR = 1 << PIO_ID;
  } else {
    AT91C_BASE_AIC->AIC_IDCR = 1 << PIO_ID;
  }
}

void hal_CC_Pin_Set(uint8_t cc_num, CC_PIN pin, GPIO_PinState state) {
  if(cc_num < CCCOUNT) {
    if(state == GPIO_PIN_SET) {
      CCtransceiver[cc_num].base[pin]->PIO_SODR = _BV(CCtransceiver[cc_num].pin[pin]);
    } else {
      CCtransceiver[cc_num].base[pin]->PIO_CODR = _BV(CCtransceiver[cc_num].pin[pin]);
    }
  }
}

uint32_t hal_CC_Pin_Get(uint8_t cc_num, CC_PIN pin) {
  if(cc_num < CCCOUNT) {
    return (CCtransceiver[cc_num].base[pin]->PIO_PDSR) & _BV(CCtransceiver[cc_num].pin[pin]);
  }
  return 0;
}


