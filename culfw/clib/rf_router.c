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

uint8_t rf_router_status;
uint8_t rf_router_myid;
uint8_t rf_router_target;
uint8_t rf_router_hsec;
uint8_t rf_router_sendtime; // relative ticks
uint8_t  rf_nr_send_checks;

#ifndef RFR_SHADOW
rb_t RFR_Buffer;
#endif

static void rf_router_send(uint8_t addAddr);
void rf_debug_out(uint8_t);

#ifdef RFR_DEBUG
uint16_t nr_t, nr_f, nr_e, nr_k, nr_h, nr_r, nr_plus;
#endif
#undef RFR_USBECHO


void
usbMsg(char *s)
{
  display_channel = DISPLAY_USB;
  display_string(s);
  DNL();
  display_channel = 0xff;
}

void
rf_router_init()
{
  rf_router_myid = erb(EE_RF_ROUTER_ID);
  rf_router_target = erb(EE_RF_ROUTER_ROUTER);
  if(rf_router_target) {
    tx_report = 0x21;
    set_txrestore();
  }
}

void
rf_router_func(char *in)
{
  if(in[1] == 0) {               // u: display id and router
    DH2(rf_router_myid);
    DH2(rf_router_target);
    DNL();
#ifdef RFR_DEBUG 
  } else if(in[1] == 'd') {     // ud: Debug
    DH((uint16_t)ticks, 4); DC('.');
    DH2(rf_router_sendtime);
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
    fromhex(in+2, &rf_router_myid, 1);
    ewb(EE_RF_ROUTER_ID, rf_router_myid);
    fromhex(in+4, &rf_router_target, 1);
    ewb(EE_RF_ROUTER_ROUTER, rf_router_target);

  } else {                      // uYYDATA: send data to node with id YY
    rb_reset(&RFR_Buffer);
    while(*++in)
      rb_put(&RFR_Buffer, *in);
    rf_router_send(0);

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

// Duration is 15ms, more than one tick!
static void
rf_router_ping(void)
{
  set_ccon();           // 1.7ms
  ccTX();               // 4.8ms

  // Sync               // 8.5ms
  for(uint8_t i = 0; i < 6; i++) {
    sethigh(RF_ROUTER_ZERO_HIGH);
    setlow(RF_ROUTER_ZERO_LOW);
  }
  sethigh(RF_ROUTER_ONE_HIGH);
  setlow(RF_ROUTER_ONE_LOW);
  sethigh(RF_ROUTER_ONE_LOW);
  SET_LOW;
}

static void
rf_router_send(uint8_t addAddr)
{
#ifdef RFR_DEBUG
       if(RFR_Buffer.buf[5] == 'T') nr_t++;
  else if(RFR_Buffer.buf[5] == 'F') nr_f++;
  else if(RFR_Buffer.buf[5] == 'E') nr_e++;
  else if(RFR_Buffer.buf[5] == 'K') nr_k++;
  else if(RFR_Buffer.buf[5] == 'H') nr_h++;
  else                              nr_r++;
#endif

  uint8_t buf[7], l = 1;
  buf[0] = RF_ROUTER_PROTO_ID;
  if(addAddr) {
    tohex(rf_router_target, buf+1);
    tohex(rf_router_myid,   buf+3),
    buf[5] = 'U';
    l = 6;
  }
  rf_router_ping();           // 15ms
  ccInitChip(EE_FASTRF_CFG);  // 1.6ms
  my_delay_ms(3);             // 3ms: Found by trial and error

  CC1100_ASSERT;
  cc1100_sendbyte(CC1100_WRITE_BURST | CC1100_TXFIFO);
#ifdef RFR_USBECHO
  uint8_t nbuf = RFR_Buffer.nbytes;
#endif
  cc1100_sendbyte(RFR_Buffer.nbytes+l);
  for(uint8_t i = 0; i < l; i++)
    cc1100_sendbyte(buf[i]);
  while(RFR_Buffer.nbytes)
    cc1100_sendbyte(rb_get(&RFR_Buffer));
  CC1100_DEASSERT;
  ccTX();
  rb_reset(&RFR_Buffer); // needed by FHT_compress

  // Wait for the data to be sent
  uint8_t maxwait = 20;        // max 20ms
  while((cc1100_readReg(CC1100_TXBYTES) & 0x7f) && maxwait--)
    my_delay_ms(1);
  set_txrestore();
#ifdef RFR_USBECHO
#warning RFR USB DEBUGGING IS ACTIVE
  uint8_t odc = display_channel;
  display_channel = DISPLAY_USB;
  DC('.'); DU(nbuf, 2); DNL();
  display_channel = odc;
#endif
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
      if(fromhex(TTY_Rx_Buffer.buf, &id, 1) == 1 &&     // it is for us
         id == rf_router_myid) {

        if(TTY_Rx_Buffer.buf[4] == 'U') {               // "Display" the data
          while(TTY_Rx_Buffer.nbytes)                   // downlink: RFR->CUL
            DC(rb_get(&TTY_Rx_Buffer));
          DNL();

        } else {                                        // uplink: CUL->RFR
          TTY_Rx_Buffer.nbytes -= 4;                    // Skip dest/src bytes
          TTY_Rx_Buffer.getoff = 4;
          rb_put(&TTY_Rx_Buffer, '\n');
          input_handle_func(DISPLAY_RFROUTER);          // execute the command
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
    rf_router_sendtime = 3;
  else
    rf_router_send(1);   // duration is more than one tick
}

#endif
