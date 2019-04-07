/* Includes ------------------------------------------------------------------*/
#include "hal_gpio.h"
#include <board.h>
#include <string.h>
#include "led.h"
#include "delay.h"
#include <pio/pio.h>
#include <aic/aic.h>
#include "multi_CC.h"

static const Pin pinsLeds[] = {PINS_LEDS};

transceiver_t CCtransceiver[] = CCTRANSCEIVERS;
uint8_t i2cPort;

/*----------------------------------------------------------------------------*/
/* USB                                                                        */
/*----------------------------------------------------------------------------*/

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
  CCtransceiver[cc_num].base[CC_Pin_In]->PIO_ODR = _BV(CCtransceiver[cc_num].pin[CC_Pin_In]);     //Enable input
  CCtransceiver[cc_num].base[CC_Pin_In]->PIO_PER = _BV(CCtransceiver[cc_num].pin[CC_Pin_In]);     //Enable PIO control

}

__attribute__((weak)) void CC1100_in_callback()
{
  /* NOTE : This function Should not be modified, when the callback is needed,
            the callback could be implemented in the user file
   */
}

void hal_GPIO_IRQHandler(uint32_t pin) {
#ifdef HAS_MULTI_CC
  uint8_t old_instance = CC1101.instance;

  if(pin & _BV(CCtransceiver[0].pin[CC_Pin_In])) {
    CC1101.instance = 0;
    CC1100_in_callback();
  }

  if(pin & _BV(CCtransceiver[1].pin[CC_Pin_In])) {
    CC1101.instance = 1;
    CC1100_in_callback();
    }
  CC1101.instance = old_instance;
#else
  CC1100_in_callback();
#endif
}

void ISR_PioA() {
  // Read PIO controller status
  hal_GPIO_IRQHandler(AT91C_BASE_PIOA->PIO_ISR);
}

#ifdef AT91C_ID_PIOB
void ISR_PioB() {
  // Read PIO controller status
  hal_GPIO_IRQHandler(AT91C_BASE_PIOB->PIO_ISR);
}
#endif

void hal_enable_CC_GDOin_int(uint8_t cc_num, uint8_t enable) {
  /*Configure in pin */

  if(!(cc_num < NUM_SLOWRF ))
    return;

  if (enable)
    CCtransceiver[cc_num].base[CC_Pin_In]->PIO_IER = _BV(CCtransceiver[cc_num].pin[CC_Pin_In]);     //Enable input change interrupt
  else
    CCtransceiver[cc_num].base[CC_Pin_In]->PIO_IDR = _BV(CCtransceiver[cc_num].pin[CC_Pin_In]);     //Disable input change interrupt

}

void HAL_GPIO_Init(void) {
  AT91C_BASE_PMC->PMC_PCER = (1 << AT91C_ID_PIOA);
  AIC_ConfigureIT(AT91C_ID_PIOA, AT91C_AIC_PRIOR_HIGHEST, ISR_PioA);
  AT91C_BASE_AIC->AIC_IECR = 1 << AT91C_ID_PIOA;

  #ifdef AT91SAM7X256
  AT91C_BASE_PMC->PMC_PCER = (1 << AT91C_ID_PIOB);
  AIC_ConfigureIT(AT91C_ID_PIOB, AT91C_AIC_PRIOR_HIGHEST, ISR_PioB);
  AT91C_BASE_AIC->AIC_IECR = 1 << AT91C_ID_PIOB;
  #endif

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

void hal_CC_move_transceiver_pins(uint8_t source, uint8_t dest) {
  transceiver_t tempCC;
  memcpy(&tempCC, &CCtransceiver[dest], sizeof(transceiver_t));
  memcpy(&CCtransceiver[dest], &CCtransceiver[source], sizeof(transceiver_t));
  memcpy(&CCtransceiver[source], & tempCC, sizeof(transceiver_t));
}

void hal_I2C_Set_Port(uint8_t port) {
  i2cPort= port;
}
