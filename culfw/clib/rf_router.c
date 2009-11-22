#include "board.h"
#ifdef HAS_RF_ROUTER
#include <string.h>
#include <avr/pgmspace.h>
#include "rf_router.h"
#include "cc1100.h"
#include "delay.h"
#include "display.h"
#include "rf_receive.h"
#include "fncollection.h"
#include "clock.h"
#include "ringbuffer.h"
#include "ttydata.h"
#include "fht.h"
#include "cdc.h"

uint8_t rf_router_status;
uint8_t rf_router_id;
uint8_t rf_router_router;
uint8_t rf_router_hsec;
uint32_t rf_router_sendtime;
rb_t RFR_Buffer;

void rf_router_ping(void);
void rf_router_send(void);
void rf_debug_out(uint8_t);

static void
rf_initbuf(void)
{
  tohex(rf_router_router, (uint8_t *)RFR_Buffer.buf);
  tohex(rf_router_id,     (uint8_t *)RFR_Buffer.buf+2);
  RFR_Buffer.buf[4] = 'U';
  RFR_Buffer.nbytes = RFR_Buffer.putoff = 5;
  RFR_Buffer.getoff = 0;
}

void
rf_router_init()
{
  rf_router_id = erb(EE_RF_ROUTER_ID);
  rf_router_router = erb(EE_RF_ROUTER_ROUTER);
  rf_initbuf();
  if(rf_router_router) {
    tx_report = 0x21;
    set_txrestore();
  }
}

void
rf_echo(char *in)
{
  while(*in)
    DC(*in++);
  DNL();
}

void
rf_router_func(char *in)
{
  if(in[1] == 0) {               // u: display id and router
    DH2(rf_router_id);
    DH2(rf_router_router);
#ifndef BUSWARE_CUL              // save mem on the CUL
    DC(' ');
    uint8_t *p;
    p = (uint8_t *)&ticks;              DH2(p[1]); DH2(p[0]); DC('.');
    p = (uint8_t *)&rf_router_sendtime; DH2(p[1]); DH2(p[0]);
#endif
    DNL();

  } else if(in[1] == 'i') {      // uiXXYY: set own id to XX and router id to YY
    fromhex(in+2, &rf_router_id, 1);
    ewb(EE_RF_ROUTER_ID, rf_router_id);
    fromhex(in+4, &rf_router_router, 1);
    ewb(EE_RF_ROUTER_ROUTER, rf_router_router);
    rf_initbuf();

  } else {                      // uYYDATA: send data to node with id YY
    rb_reset(&RFR_Buffer);
    in++;
    while(*in)
      rb_put(&RFR_Buffer, *in++);
    rf_router_send();

  }
}


#define RF_ROUTER_ZERO_HIGH 384
#define RF_ROUTER_ZERO_LOW  768
#define RF_ROUTER_ONE_HIGH  768
#define RF_ROUTER_ONE_LOW   384
#define SET_HIGH CC1100_OUT_PORT |= _BV(CC1100_OUT_PIN)
#define SET_LOW  CC1100_OUT_PORT &= ~_BV(CC1100_OUT_PIN)

void
rf_router_ping(void)
{
  set_ccon();
  ccTX();

  // Sync
  for(uint8_t i = 0; i < 6; i++) {
    SET_HIGH; my_delay_us(RF_ROUTER_ZERO_HIGH);
    SET_LOW;  my_delay_us(RF_ROUTER_ZERO_LOW);
  }
  SET_HIGH; my_delay_us(RF_ROUTER_ONE_HIGH);
  SET_LOW;  my_delay_us(RF_ROUTER_ONE_LOW);
  SET_HIGH; my_delay_us(RF_ROUTER_ONE_LOW);
  SET_LOW;
}

void
rf_router_send()
{
  rf_router_ping();
  ccInitChip(EE_FASTRF_CFG);
  my_delay_ms(3);             // 3ms: Found by trial and error
  CC1100_ASSERT;
  cc1100_sendbyte(CC1100_WRITE_BURST | CC1100_TXFIFO);
  cc1100_sendbyte( RFR_Buffer.nbytes );
  while(RFR_Buffer.nbytes)
    cc1100_sendbyte(rb_get(&RFR_Buffer));
  CC1100_DEASSERT;
  ccTX();

  // Wait for the data to be sent
  uint8_t maxwait = 20;        // max 20ms
  while(cc1100_readReg(CC1100_TXBYTES) & 0x7f && maxwait--)
    my_delay_ms(1);
  set_txrestore();
  rf_initbuf();
}

void
rf_router_task(void)
{
  if(rf_router_status == RF_ROUTER_INACTIVE)
    return;

  uint8_t hsec = (uint8_t)ticks;

  if(rf_router_status == RF_ROUTER_GOT_DATA) {

    uint8_t len = cc1100_readReg(CC1100_RXFIFO)+2;
    if(len < TTY_BUFSIZE && len > 4) {

      rb_reset(&TTY_Rx_Buffer);
      CC1100_ASSERT;
      cc1100_sendbyte( CC1100_READ_BURST | CC1100_RXFIFO );
      while(len-- > 2)
        rb_put(&TTY_Rx_Buffer, cc1100_sendbyte(0));
      cc1100_sendbyte(0);       // RSSI
      cc1100_sendbyte(0);       // LQI
      CC1100_DEASSERT;

      uint8_t id;
      if(fromhex(TTY_Rx_Buffer.buf, &id, 1) == 1 &&
         id == rf_router_id) {

        if(TTY_Rx_Buffer.buf[4] == 'U') {
          while(TTY_Rx_Buffer.nbytes)
            DC(rb_get(&TTY_Rx_Buffer));
          DNL();

        } else {
          TTY_Rx_Buffer.nbytes -= 4;    // Skip dest/src bytes
          TTY_Rx_Buffer.getoff = 4;
          rb_put(&TTY_Rx_Buffer, '\n');
          uint8_t odc = display_channel;
          display_channel = DISPLAY_RFROUTER;
          analyze_ttydata();
          display_channel = odc;
        }

      } else {
        rb_reset(&TTY_Rx_Buffer);
      }

    }

    set_txrestore();
    rf_router_status = RF_ROUTER_INACTIVE;

  } else if(rf_router_status == RF_ROUTER_DATA_WAIT) {
    uint8_t diff = hsec - rf_router_hsec;
    if(diff > 7) {              // 3 (delay above) + 3 ( ((4+64)*8)/250kBaud )
      set_txrestore();
      rf_router_status = RF_ROUTER_INACTIVE;
    }

  } else if(rf_router_status == RF_ROUTER_SYNC_RCVD) {
    ccInitChip(EE_FASTRF_CFG);
    ccRX();
    rf_router_status = RF_ROUTER_DATA_WAIT;
    rf_router_hsec = hsec;

  } 
}

void
rf_router_flush()
{
  if(RFR_Buffer.nbytes == 0)
    return;
  if(rf_isreceiving()) {
    rf_router_sendtime = ticks+5;     // 32-40ms, to avoid FS20 collision
    return;
  }
  rf_router_send();
}

void
rf_debug_out(uint8_t c)
{
  uint8_t odc = display_channel;
  display_channel = DISPLAY_USB;
  DH2(c); CDC_Task();
  display_channel = odc;
}

#endif
