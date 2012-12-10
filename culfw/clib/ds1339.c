#include "i2cmaster.h"
#include "board.h"
#include "display.h"
#include "ds1339.h"
#include <avr/interrupt.h>
#include <avr/io.h>


unsigned char rtc_read(unsigned char addr);
void rtc_write(unsigned char addr, unsigned char data);

unsigned char
rtc_read(unsigned char addr)
{
  unsigned char ret;
  i2c_start_wait(RTC_ADDR); // set device address and write mode
  i2c_write(addr);          // write address = 0
  i2c_rep_start(RTC_ADDR+1);// set device address and read mode
  ret = i2c_readNak();      // read one byte form address 0
  i2c_stop();               // set stop condition = release bus
  return ret;
}

void
rtc_write(unsigned char addr, unsigned char data)
{
  i2c_start_wait(RTC_ADDR); // set device address and write mode
  i2c_write(addr);          // write address
  i2c_write(data);          // ret=0 -> Ok, ret=1 -> no ACK 
  i2c_stop();               // set stop conditon = release bus
}

void
rtc_init(void)
{
  i2c_init();               // init I2C interface
  rtc_write( 0x10, 0xa5 );  // TCS3,TCS1,DS0,ROUT0 (No diode, 250Ohm)
  rtc_write( 0x0e, 0x18 ); 

#if 0
  uint8_t hb[6];
  rtc_dotime(1, hb);
  rtc_write(0x7, (hb[5]+1)|0x80); 
  rtc_write(0x8, hb[4]|0x80);  
  rtc_write(0x9, hb[3]|0x80);
  rtc_write(0xa, hb[2]|0x80);

  rtc_write(0xb, (hb[4]+1)|0x80);     // Alarm2 set to once every minute
  rtc_write(0xc, hb[3]|0x80);
  rtc_write(0xd, hb[2]|0x80);
  rtc_write( 0x0e, 0x07 );  // INTCN & A2IE : Interrupt & Alarm2 Enable

  RTC_DDR &= ~_BV(RTC_PIN);
  RTC_OUT_PORT |= _BV(RTC_PIN);
  RTC_INTREG &= ~ISC41;
  RTC_INTREG |= ISC40;
  EIMSK |= _BV(RTC_INT);
  EIFR  |= _BV(RTC_INT);
#endif
}

void
rtc_get(uint8_t data[6])
{
  data[0] = rtc_read(6);         // year
  data[1] = rtc_read(5);         // month
  data[2] = rtc_read(4);         // mday
  data[3] = rtc_read(2);         // hour
  data[4] = rtc_read(1);         // min
  data[5] = rtc_read(0);         // sec
}

void
rtc_set(uint8_t len, uint8_t data[6])
{
  uint8_t p = 0;
  if(len == 6) {
    rtc_write(6, data[p++]);
    rtc_write(5, data[p++]);
    rtc_write(4, data[p++]);
  }
  rtc_write(2, data[p++]);
  rtc_write(1, data[p++]);
  rtc_write(0, data[p++]);
}


void
rtc_func(char *in)
{
  uint8_t hb[6], t;

  t = fromhex(in+1, hb, 6);
  if(t < 3) {
    t = hb[0];
    rtc_get(hb);

    if(t&1) {
      DH2(hb[0]); DC('-');
      DH2(hb[1]); DC('-');
      DH2(hb[2]);
    }

    if((t&3) == 3)
      DC(' ');

    if(t&2) {
      DH2(hb[3]); DC(':');
      DH2(hb[4]); DC(':');
      DH2(hb[5]);
    }
    DNL();
  }

  else  {       // 3: time only, 6: date & time
    rtc_set(t, hb);

  }

}

#if 0
ISR(RTC_INTVECT)
{
  DC('X');
  DNL();
}
#endif
