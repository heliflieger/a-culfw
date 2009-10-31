#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>

#include "board.h"
#include "led.h"
#include "fncollection.h"
#include "clock.h"
#include "display.h"
#include "battery.h"
#include "joy.h"
#include "fht.h"
#include "fswrapper.h"                 // fs_sync();
#include "rf_send.h"                   // credit_10ms
#ifdef HAS_SLEEP
#include "mysleep.h"
#endif
#ifdef HAS_LCD
#include "pcf8833.h"
#endif
#ifdef HAS_USB
#include "cdc.h"
#endif
#include "ntp.h"

uint32_t ticks;
#if defined(HAS_LCD) && defined(BAT_PIN)
#endif

#ifdef HAS_ETHERNET
uint32_t ticks;
#endif
	
// count & compute in the interrupt, else long runnning tasks would block
// a "minute" task too long
ISR(TIMER0_COMPA_vect, ISR_BLOCK)
{

  ticks++;

#ifdef HAS_NTP
  ntp_hsec++;
  if(ntp_hsec >= 125) {
    ntp_sec++;
    ntp_hsec = 0;
  }
#endif

#ifdef HAS_FHT_8v
  if(fht8v_timeout)
    fht8v_timeout--;
#endif
#ifdef HAS_FHT_80b
  if(fht80b_timer_enabled && fht80b_timeout)
    fht80b_timeout--;
#endif


}

void
gettime(char *unused)
{
  uint8_t *p = (uint8_t *)&ticks;
  DH2(p[3]);
  DH2(p[2]);
  DH2(p[1]);
  DH2(p[0]);
  DNL();
}

#ifdef HAS_ETHERNET
//Return time
clock_time_t
clock_time()
{
  return (clock_time_t)ticks;
}
#endif

void
Minute_Task(void)
{
  static uint8_t last_tick, clock_hsec;
  if((uint8_t)ticks == last_tick)
    return;
  last_tick = (uint8_t)ticks;
  wdt_reset();

#ifdef HAS_FHT_8v
  if(fht8v_timeout == 0)
    fht8v_timer();
#endif
#ifdef HAS_FHT_80b
  if(fht80b_timer_enabled && fht80b_timeout == 0)
    fht80b_timer();
#endif

  if(clock_hsec++ < 125)     // Note: this can skip some hsecs
    return;
  clock_hsec = 0;

  if(led_mode & 2)
    LED_TOGGLE();

  // one second, 1% duty cycle, 10ms resolution => this is simple ;-)
  if (credit_10ms < MAX_CREDIT)
    credit_10ms += 1;

#if defined(HAS_SLEEP) && defined(JOY_PIN1)
  if(joy_inactive < 255)
    joy_inactive++;

  if(sleep_time && joy_inactive == sleep_time) {
    if(USB_IsConnected)
      lcd_switch(0);
    else
      dosleep();
  }
#endif

#ifdef HAS_NTP
  if((ntp_sec & NTP_INTERVAL_MASK) == 0)
    ntp_sendpacket();
#endif

#if defined(HAS_LCD) && defined(BAT_PIN)
  static uint8_t clock_sec;
  clock_sec++;
  if(clock_sec != 60)                   // minute from now on
    return;
  clock_sec = 0;

  bat_drawstate();
#endif
}
