#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/wdt.h>

#include "board.h"
#include "util.h"
#include "cc1100.h"
#include "fs20.h"

/*
 * -----------------------------------------------------------------
 * 
 * hardware 
 * 
 * -----------------------------------------------------------------
 */

/*
 * init the device
 */
void
hardware_init (void)
{
  // init LED
  LED_INIT ();
  LED_OFF();

  // init switch
  SW_INIT ();

  // SPI
  // MOSI, SCK, als Output / MISO als Input
  SPI_DDR |= _BV (SPI_MOSI) | _BV (SPI_SCLK);	// mosi, sck output
  SPI_DDR &= ~_BV (SPI_MISO);	// miso input

  // test pin change interrupt
  
  /* ATTiny84 manual p. 50
   * When the PCIE0 bit is set (one) and the I-bit in the Status Register (SREG) is set (one), pin
   * change interrupt 0 is enabled. Any change on any enabled PCINT7..0 pin will cause an interrupt.
   * The corresponding interrupt of Pin Change Interrupt Request is executed from the PCI0 Interrupt
   * Vector. PCINT7..0 pins are enabled individually by the PCMSK0 Register.
   */
  GIMSK |= _BV (PCIE0);
  
  
  /* ATTiny manual p. 52
   * Each PCINT7..0 bit selects whether pin change interrupt is enabled on the corresponding I/O
   * pin. If PCINT7..0 is set and the PCIE0 bit in GIMSK is set, pin change interrupt is enabled on the
   * corresponding I/O pin. If PCINT7..0 is cleared, pin change interrupt on the corresponding I/O pin
   * is disabled.
   * 
   * PCINT0 <-> PA0
   * ...
   * PCINT7 <-> PA7
   */
  PCMSK0 = MASK0;	/* pcint0 + 1 aktivieren */


#ifdef USE_TIMER
  disable_wdi();
  // enable timer interrupt
  // start Timer0: 0.008s = 250*256/8MHz
  OCR0A  = 250;           // count from 0 to 250
  TCCR0B = _BV(CS02);     // bit mask 100: Prescaler= 256
  TCCR0A = _BV(WGM01);    // wave form generation mode 01
  TIMSK0 = _BV(OCIE0A);   // Timer 0 Output Compare A Match Interrupt Enable
#else
  // enable watchdog interrupt
  enable_wdi ();
#endif
  
  sei();
  
  // CC1100 subsystem
  ccInitChip ();
  fs20_init ();
}


/*
 * make LED blink num times
 */
void
led_blink (uint8_t num)
{

  for (uint8_t i = 0; i < num * 2; i++)
    {
      LED_TOGGLE ();
      _delay_ms (100);
    }
}

/*
 * get key press 
 * 0= no key press
 * 1= short key press
 * 2= long key press
 */
uint8_t
get_keypress(uint16_t longduration) {
  #define GRANULARITY (10)
  uint16_t duration= 0;
  while(SW_STATE()) {
    duration+= GRANULARITY;
    _delay_ms(GRANULARITY);
    if(duration >= longduration) LED_ON();
  }
  LED_OFF();
  if(duration) {
    if(duration>= longduration) return 2;
    return 1;
  }
  return 0;
}

/*
 * -----------------------------------------------------------------
 * 
 * power management
 * 
 * -----------------------------------------------------------------
 */

/*
 * disable watchdog timer interrupt
 */
void
disable_wdi(void) {
  // disable watchdog
  MCUSR &= ~_BV (WDRF);
  WDTCSR |= _BV (WDCE) | _BV (WDE);
  WDTCSR &= ~_BV (WDE);
}
  

/*
 * enable watchdog timer interrupt
 */
void
enable_wdi (void)
{
  // see p44 in ATtiny84 manual
  disable_wdi();
  
  // configure watchdog
  // 8.0 sec
  #define WDTIMEOUT 8
  WDTCSR |= _BV (WDP3) | _BV (WDP0);
  WDTCSR &= ~(_BV (WDP2) | _BV (WDP1));
  /*
   * alternatives:
   * 
  // 4.0 sec
  WDTCSR |= _BV(WDP3); WDTCSR &= ~ ( _BV(WDP2) | _BV(WDP1) | _BV(WDP0) );
  // 1.0 sec
  WDTCSR |= _BV(WDP2) | _BV(WDP1); WDTCSR &= ~ ( _BV(WDP3) | _BV(WDP0) );
  */
  // enable watchdog timeout interrupt
  WDTCSR |= _BV (WDIE);
}


/*
 * power down CC1101 and ATtiny84
 */
void
power_down (void)
{

  // turn CC1101 into power down mode
  ccStrobe (CC1100_SPWD);
  //ccStrobe(CC1100_SWOR);
  
  // disable ADC if not already disabled
  ADCSRA &= ~_BV (ADEN);
  // disable AC
  ACSR |= _BV (ACD);

  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_mode ();
}

/*
 * -----------------------------------------------------------------
 * 
 * FS20 
 * 
 * -----------------------------------------------------------------
 */
 
/*
 * send FS20 command datagram
 */
void
fs20_sendCommand (uint16_t housecode, uint8_t button, uint8_t cmd)
{
  ccInitChip();

  uint8_t data[4];
  data[0] = housecode >> 8;
  data[1] = housecode & 0xff;
  data[2] = button;
  data[3] = cmd;
  fs20_send (data, 4);
}

/*
 * send FS20 sensor datagram
 */
void
fs20_sendValue (uint16_t housecode, uint8_t sensor, uint8_t flags,
	       uint16_t value)
{
  ccInitChip();

  uint8_t data[5];
  uint16_t data10 = value & 0x3ff;	// 10 bit reading
  data[0] = housecode >> 8;
  data[1] = housecode & 0xff;
  data[2] = sensor;
  // fff1ffdd dddddddd
  data[3] = flags | 0x20 | (data10 >> 8);
  data[4] = (data10 & 0xff);
  fs20_send (data, 5);
}

void
fs20_sendValues (uint16_t housecode, uint8_t sensor,
	       uint16_t value1, uint16_t value2)
{
  ccInitChip();

  uint8_t data[7];
  data[0] = housecode >> 8;
  data[1] = housecode & 0xff;
  data[2] = sensor;
  data[3] = value1 >> 8;
  data[4] = value1 & 0xff;
  data[5] = value2 >> 8;
  data[6] = value2 & 0xff;
  fs20_send (data, 7);
}

void
fs20_send3Values (uint16_t housecode, uint8_t sensor,
	       uint16_t value1, uint16_t value2, uint16_t value3)
{
  ccInitChip();

  uint8_t data[9];
  data[0] = housecode >> 8;
  data[1] = housecode & 0xff;
  data[2] = sensor;
  data[3] = value1 >> 8;
  data[4] = value1 & 0xff;
  data[5] = value2 >> 8;
  data[6] = value2 & 0xff;
  data[7] = value3 >> 8;
  data[8] = value3 & 0xff;
  fs20_send (data, 9);
}


/*
 * -----------------------------------------------------------------
 * 
 * Timer 
 * 
 * -----------------------------------------------------------------
 */

volatile uint32_t clock;	// in seconds
volatile uint32_t ticks;	// in 125ths of a second

void reset_clock(void) {
    clock= 0;
    ticks= 0;
}

uint32_t tick_tick(void) {
    // 125 ticks per second
    ticks++;
    if(ticks>= 125) {
	clock++; 
	ticks= 0;
    }
    return ticks;
}

uint32_t tick_clock(void) {
    return clock+= WDTIMEOUT;
}

uint32_t get_clock(void) {
    return clock*TIMERCORR;
}