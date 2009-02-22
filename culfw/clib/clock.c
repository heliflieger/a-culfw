#include <avr/io.h>
#include <avr/wdt.h>

#include "led.h"
#include "fncollection.h"
#include "clock.h"
#include "display.h"
#include "transceiver.h"
#include "battery.h"
#include "joy.h"
#include "fht.h"
#ifdef HAS_SLEEP
#include "mysleep.h"
#endif
#ifdef HAS_LCD
#include "pcf8833.h"
#endif
#include <MyUSB/Drivers/USB/USB.h>     // USB Functionality

uint8_t day,hour,minute,sec,hsec;
static uint8_t lsec, lmin;

//Counted time
//clock_time_t clock_datetime = 0;
	
// count & compute in the interrupt, else long runnning tasks would block
// a "minute" task too long
ISR(TIMER0_COMPA_vect, ISR_BLOCK)
{
  hsec++;

  if(hsec >= 125) {

    //clock_datetime++;

    sec++; hsec = 0;
    if(sec == 60) {
      minute++; sec = 0;
      if(minute == 60) {
        hour++; minute = 0;
        if(hour == 24) {
          day++; hour = 0;
        }
      }
    }
  }
}

void
gettime(char *unused)
{
  display_udec(day,3,'0');     DC(' ');
  display_udec(hour, 2,'0');   DC(':');
  display_udec(minute, 2,'0'); DC(':');
  display_udec(sec, 2,'0');
  DNL();
}

#if 0
//Return time
clock_time_t
clock_time()
{
  clock_time_t time;
  cli();
  time = clock_datetime;
  sei();
  return time;
}
#endif

TASK(Minute_Task)
{
  if(lsec == sec)
    return;

  lsec = sec;

  wdt_reset();

  if(led_mode & 2)
    LED_TOGGLE();

  // one second, 1% duty cycle, 10ms resolution => this is simple ;-)
  if (credit_10ms < MAX_CREDIT)
    credit_10ms += 1;

#ifdef HAS_FHT_8v
  if(fht8v_timeout != 0xff && --fht8v_timeout == 0)
    fht_timer();
#endif

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

  if(lmin == minute)
    return;
  lmin = minute;

#if defined(HAS_LCD) && defined(BAT_PIN)
  bat_drawstate();
#endif

#ifdef BUSWARE_CUL
  if(!USB_IsConnected)  // kind of a watchdog
    prepare_boot(0);
#endif

}
