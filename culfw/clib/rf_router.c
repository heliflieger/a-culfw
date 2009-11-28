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
uint8_t  rf_nr_send_checks;
rb_t RFR_Buffer;

void rf_router_send(void);
void rf_debug_out(uint8_t);

#define RFR_DEBUG
#ifdef RFR_DEBUG
uint16_t nr_t, nr_f, nr_e, nr_k, nr_h, nr_r, nr_plus;
#endif


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
rf_router_func(char *in)
{
  if(in[1] == 0) {               // u: display id and router
    DH2(rf_router_id);
    DH2(rf_router_router);
    DNL();
#ifdef RFR_DEBUG 
  } else if(in[1] == 'd') {     // ud: Debug
    DH((uint16_t)ticks, 4); DC('.');
    DH((uint16_t)rf_router_sendtime, 4);
    DNL();
  } else if(in[1] == 's') {     // us: Statistics
    DH(nr_t,1); DC('.');
    DH(nr_f,1); DC('.');
    DH(nr_e,1); DC('.');
    DH(nr_k,1); DC('.');
    DH(nr_h,1); DC('.');
    DH(nr_r,1); DC('.');
    DH(nr_plus,1);
    DNL();
#endif

  } else if(in[1] == 'i') {      // uiXXYY: set own id to XX and router id to YY
    fromhex(in+2, &rf_router_id, 1);
    ewb(EE_RF_ROUTER_ID, rf_router_id);
    fromhex(in+4, &rf_router_router, 1);
    ewb(EE_RF_ROUTER_ROUTER, rf_router_router);
    rf_initbuf();

  } else {                      // uYYDATA: send data to node with id YY
    rb_reset(&RFR_Buffer);
    while(*++in)
      rb_put(&RFR_Buffer, *in);
    rf_router_send();

  }
}


#define RF_ROUTER_ZERO_HIGH 384
#define RF_ROUTER_ZERO_LOW  768
#define RF_ROUTER_ONE_HIGH  768
#define RF_ROUTER_ONE_LOW   384
#define SET_HIGH CC1100_OUT_PORT |= _BV(CC1100_OUT_PIN)
#define SET_LOW  CC1100_OUT_PORT &= ~_BV(CC1100_OUT_PIN)
void sethigh(uint16_t dur) { SET_HIGH; my_delay_us(dur); }
void setlow(uint16_t dur) { SET_LOW; my_delay_us(dur); }

// Duration is 8.448ms, more than one tick!
static void
rf_router_ping(void)
{
  set_ccon();
  ccTX();

  // Sync
  for(uint8_t i = 0; i < 6; i++) {
    sethigh(RF_ROUTER_ZERO_HIGH);
    setlow(RF_ROUTER_ZERO_LOW);
  }
  sethigh(RF_ROUTER_ONE_HIGH);
  setlow(RF_ROUTER_ONE_LOW);
  sethigh(RF_ROUTER_ONE_LOW);
  SET_LOW;
}

void
rf_router_send()
{
#ifdef RFR_DEBUG
#warning RF_ROUTER RFR_DEBUG enabled
       if(RFR_Buffer.buf[5] == 'T') nr_t++;
  else if(RFR_Buffer.buf[5] == 'F') nr_f++;
  else if(RFR_Buffer.buf[5] == 'E') nr_e++;
  else if(RFR_Buffer.buf[5] == 'K') nr_k++;
  else if(RFR_Buffer.buf[5] == 'H') nr_h++;
  else                              nr_r++;
#endif
  rf_router_ping();
  ccInitChip(EE_FASTRF_CFG);
  my_delay_ms(3);             // 3ms: Found by trial and error
  CC1100_ASSERT;
  cc1100_sendbyte(CC1100_WRITE_BURST | CC1100_TXFIFO);
  cc1100_sendbyte( RFR_Buffer.nbytes+1 );
  cc1100_sendbyte( RF_ROUTER_PROTO_ID );
  while(RFR_Buffer.nbytes)
    cc1100_sendbyte(rb_get(&RFR_Buffer));
  CC1100_DEASSERT;
  ccTX();

  // Wait for the data to be sent
  uint8_t maxwait = 20;        // max 20ms
  while((cc1100_readReg(CC1100_TXBYTES) & 0x7f) && maxwait--)
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

    uint8_t len = cc1100_readReg(CC1100_RXFIFO);
    uint8_t proto = 0;

    if(len > 5) {
      rb_reset(&TTY_Rx_Buffer);
      CC1100_ASSERT;
      cc1100_sendbyte( CC1100_READ_BURST | CC1100_RXFIFO );
      proto = cc1100_sendbyte(0);
      while(--len)
        rb_put(&TTY_Rx_Buffer, cc1100_sendbyte(0));
      CC1100_DEASSERT;
    }
    set_txrestore();
    rf_router_status = RF_ROUTER_INACTIVE;

    if(proto == RF_ROUTER_PROTO_ID) {
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
          input_handle_func();
          display_channel = odc;
        }

      } else {
        rb_reset(&TTY_Rx_Buffer);
      }
    }

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
  if(rf_isreceiving()) {
    rf_nr_send_checks = 3;
#ifdef RFR_DEBUG
    nr_plus++;
#endif
  }

  if(--rf_nr_send_checks)
    rf_router_sendtime = ticks+3;
  else
    rf_router_send();   // duration is more than one tick
}

#endif
