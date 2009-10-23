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

uint8_t rf_router_triggered;
uint8_t rf_router_status;
uint8_t rf_router_id;
uint8_t rf_router_router;
uint8_t rf_router_hsec;
uint8_t rf_router_data[32];
uint8_t rf_dlen;

void rf_router_ping(void);

void
rf_router_init()
{
  rf_router_id = erb(EE_RF_ROUTER_ID);
  rf_router_router = erb(EE_RF_ROUTER_ROUTER);
}

void
rf_router_func(char *in)
{
  if(in[1] == 0) {               // u: display id and router
    DH2(rf_router_id);
    DC(' ');
    DH2(rf_router_router);
    DNL();

  } else if(in[1] == 'i') {      // uiXXYY: set own id to XX and router id to YY
    fromhex(in+2, &rf_router_id, 1);
    ewb(EE_RF_ROUTER_ID, rf_router_id);
    fromhex(in+4, &rf_router_router, 1);
    ewb(EE_RF_ROUTER_ROUTER, rf_router_router);

  } else {                      // uYYDATA: send data to node with id YY
    rf_router_ping();
    ccInitChip(EE_FASTRF_CFG);
    my_delay_ms(1);
    in++;
    for(rf_dlen = 0; rf_dlen < sizeof(rf_router_data) && *in; rf_dlen++)
      rf_router_data[rf_dlen] = *in++;
    rf_router_send();
    rf_router_status = RF_ROUTER_ACK_WAIT;
    rf_router_hsec = (uint8_t)ticks;
    ccRX();
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
  if(rf_router_status != RF_ROUTER_DISABLED)
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
  CC1100_ASSERT;
  cc1100_sendbyte(CC1100_WRITE_BURST | CC1100_TXFIFO);
  cc1100_sendbyte( rf_dlen );
  for(uint8_t i=0; i < rf_dlen; i++)
    cc1100_sendbyte(rf_router_data[i]);
  CC1100_DEASSERT;
  ccTX();
  while(cc1100_readReg(CC1100_TXBYTES) & 0x7f)  // Wait for the data to be sent
    my_delay_us(100);
  ccRX();                          // set reception again. MCSM1 does not work.
}

void
rf_router_task(void)
{
  uint8_t hsec = (uint8_t)ticks;
  uint8_t diff = hsec - rf_router_hsec;

  if(rf_router_status == RF_ROUTER_DATA_WAIT) {
    if(diff > 5) {
      DC('a'); DNL();
      set_txrestore();
      rf_router_status = RF_ROUTER_DISABLED;
    }

  } else if(rf_router_status == RF_ROUTER_ACK_WAIT) {
    if(diff > 5) {
      DC('w'); DNL();
      set_txrestore();
      rf_router_status = RF_ROUTER_DISABLED;
    }

  } else if(rf_router_status == RF_ROUTER_SYNC_RCVD) {
    DC('~');
    ccInitChip(EE_FASTRF_CFG);
    ccRX();
    rf_router_status = RF_ROUTER_DATA_WAIT;
    rf_router_hsec = hsec;

  } else if(rf_router_triggered) {

DC('u');
    uint8_t len = cc1100_readReg(CC1100_RXFIFO)+2;
    rf_router_triggered = 0;
DH2(len);
    if(len < sizeof(rf_router_data)) {
      uint8_t i;

      CC1100_ASSERT;
      cc1100_sendbyte( CC1100_READ_BURST | CC1100_RXFIFO );
      for(i=0; i<len; i++)
        rf_router_data[i] = cc1100_sendbyte( 0 );
      CC1100_DEASSERT;
      rf_dlen = len;

      for(i=0; i<len-2; i++)
        DC(rf_router_data[i]);
      DC('.');
      DH2(rf_router_data[i++]);
      DH2(rf_router_data[i]);
    }

    if(rf_router_status == RF_ROUTER_DATA_WAIT) {
      ccTX();
      my_delay_ms(1);
      tohex(rf_router_id, rf_router_data);
      rf_dlen = 2;
      rf_router_send();

    } else if(len == 2) {       // ACK_WAIT
      DC('A');

    }
    rf_router_status = RF_ROUTER_DISABLED;
    set_txrestore();
    DNL();
  }


}

#endif
