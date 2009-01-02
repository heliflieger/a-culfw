#include <avr/eeprom.h>
#include "board.h"
#include "transceiver.h"
#include "fncollection.h"
#include "display.h"
#include "fht.h"
#include "delay.h"
#include "cc1100.h"

#define FHT_CSUM_START          12
#define FHT_CAN_XMIT          0x53
#define FHT_CAN_RCV           0x54
#define FHT_ACK               0x4B
#define FHT_START_XMIT        0x7D
#define FHT_END_XMIT          0x7E
#define FHT_ACTUATOR_INTERVAL  116       // in seconds

#define XS_IDLE                  0
#define XS_WAITFOR_ACKRCV        1
#define XS_WAITFOR_ACKXMIT       3
#define XS_WAITFOR_ACKDATA       4

// We have two different work models:
// 1. Control the FHT80b. In this mode we have to wait for a message from the
//    FHT80b, and then send all buffered messages to it, with a strange
//    ask/reply protocol.
// 2. Control the valves directly (8v mode). In this mode we have to send every
//    116 second a message to the valves. Some messages (e.g. pair) are sent
//    directly. This mode is activated when we receive an actuator message for
//    our own housecode.


uint8_t fht_hc[2];
uint8_t fht8v_timeout = 0xff;      
uint8_t fht80b_xstate = XS_IDLE,        // 80b state machine
        fht80b_xid[2];                  // id of current FHT80b partnerr

uint8_t fht_d1 = FHT_ACTUATOR_INTERVAL,
        fht_d2 = 0, // Delay before
        fht_d3 = 2, // repetitions
        fht_d4 = 0; // delay after

#if 0

74 00 05 00     1      0  66 132 198 264
74 00 0A 00     2      0  66 132 198 264 330 396 462  528  594
74 00 0A 30     3      0 114 228 342 456 570 684 798  912 1026
74 00 05 30     1      0 114 228 342 456 

74 00 01 A2     1      0
74 00 02 A2     1      0 228
74 00 03 A2     2      0 228 456
74 00 04 A2     2      0 228 456 684
74 00 05 A2     3      0 228 456 684 912 
74 00 07 A2     4      0 228 456 684 912 1140 1368 

74 00 02 A2     1      0 228
   05           0      5 232

74 00 02 A2     1        0 228
   E4           1      224 456
   E4           0      224 456

74 00 02 64     1        0 160
   A0           0      160 320

74 00 02 86     1        0 200
   C8           0      200 400

74 00 02 A2     1        0 228
   E4           0      162 390

74 00 02 A0     1        0 226  d2*=8
   1C           1      224 450 
   38           1      448 674 
   54           0      672 898

74 00 02 A0     1        0 226 
   1C           1      224 450 
   38           1      448 674 
   70           0      896 1122


74 00 0A 42     3      0 132 264 396 528 660 792 924 1056 1188

74 00 02 FF     1      0 255
74 00 03 FF     1      0 255 512

#endif


static void
set_fht8v_timer(void)
{
  uint8_t i;
  for(i = 0; i < 9; i++)
    if(erb(EE_FHT_ACTUATORS+2*i) != 0xff)
      break;
  if(i == 9)
    fht8v_timeout = 0xff;
  else if(fht8v_timeout == 0xff)
    fht8v_timeout = fht_d1;
}

void
fht_init(void)
{
  fht_hc[0] = erb(EE_FHTID);
  fht_hc[1] = erb(EE_FHTID+1);
  set_fht8v_timer();
  fht80b_xstate = XS_IDLE;
}


void
fhtsend(char *in)
{
  uint8_t hb[6];

  if(fromhex(in+1, hb, 6) < 3)
    return;
  if(hb[0] == 0xff && hb[1] == 0xff) {        // Control commands

    if(hb[2] == 1) {                          // Set housecode, clear buffers

      fht_hc[0] = hb[3];
      fht_hc[1] = hb[4];
      ewb(EE_FHTID  , fht_hc[0]);
      ewb(EE_FHTID+1, fht_hc[1]);

      for(uint8_t i = 0; i < 18; i+=2)
        ewb(EE_FHT_ACTUATORS+i, 0xff);
      for(uint8_t i = 0; i < EE_FHTBUF_SIZE; i++)
        ewb(EE_START_FHTBUF+i, 0xff);

      fht8v_timeout = 0xff;
      fht80b_xstate = XS_IDLE;
    }

    if(hb[2] == 2) {                          // Own housecode: 
      DH(fht_hc[0], 2);                       // 1.st byte: 80b relevant
      DH(fht_hc[1], 2);                       // 1.st+2.nd byte: 8v relevant
      DNL();
    }

    if(hb[2] == 3) {                          // Return the 8v buffer
      uint8_t na=0, v, i = 8;
      do {
        if((v = erb(EE_FHT_ACTUATORS+2*i)) == 0xff)
          continue;
        if(na)
          DC(' ');
        DH(i, 2); DC(':');
        DH(v,2);
        DH(erb(EE_FHT_ACTUATORS+2*i+1),2);
        na++;
      } while(i--);

      if(na==0)
        DS_P( PSTR("N/A") );
      DNL();
    }

    if(hb[2] == 4) {                          // Return the 80b buffer
      uint8_t na=0, v;
      for(uint8_t i = 0; i < EE_FHTBUF_SIZE; i += 5) {
        if((v = erb(EE_START_FHTBUF+i)) == 0xff)
          continue;
        if(na)
          DC(' ');
        DH(v, 2);
        for(uint8_t j = 1; j < 5; j++)
          DH(erb(EE_START_FHTBUF+i+j), 2);
        na++;
      }
      if(na==0)
        DS_P( PSTR("N/A") );
      DNL();
    }

    if(hb[2] == 5) {                          // Return the next 8v timeout
      DU(fht8v_timeout, 2);
      DNL();
    }

#if 1
    if(hb[2] == 6) {                          // Internal state machine
      DH(fht80b_xstate, 2);
      DH(fht80b_xid[0], 2);
      DH(fht80b_xid[1], 2);
      DNL();
    }

    if(hb[2] == 7) {                          // Parameters
      fht_d1 = hb[3];
      fht_d3 = hb[4];
      DH(fht_d1, 2);
      DH(fht_d3, 2);
      DNL();
    }
    if(hb[2] == 8) {                          // Parameters
      fht_d2 = hb[3];
      fht_d4 = hb[4];
      DH(fht_d2, 2);
      DH(fht_d4, 2);
      DNL();
    }
#endif



  } else if(hb[0]==fht_hc[0] && hb[1]==fht_hc[1]) {  // FHT8v mode commands

    if(hb[3] == 0x2f ||                        // Pair: Send immediately
       hb[3] == 0x20 ||                        // Syncnow: Send immediately
       hb[3] == 0x2c) {                        // Sync: Send immediately

      addParityAndSend(in, FHT_CSUM_START, 2);

    } else {                                  // Store the rest

      uint8_t a = (hb[2] < 9 ? hb[2] : 0);
      ewb(EE_FHT_ACTUATORS+a*2,  hb[3]);      // Command or 0xff for disable
      ewb(EE_FHT_ACTUATORS+a*2+1,hb[4]);      // value
      set_fht8v_timer();

    }

  } else {                                    // FHT80b mode: Queue everything

    for(uint8_t i = 0; i < EE_FHTBUF_SIZE; i += 5) {  // Look for an empty slot
      if(erb(EE_START_FHTBUF+i) != 0xff)
        continue;
      for(uint8_t j = 0; j < 5; j++) {
        ewb(EE_START_FHTBUF+i+j, hb[j]);
      }
      break;
    }

  }

}


void
fht_timer(void)
{
  uint8_t hb[6], i = 8;

  hb[0] = fht_hc[0];
  hb[1] = fht_hc[1];

  my_delay_ms(fht_d2*8);
  do {
    if((hb[3] = erb(EE_FHT_ACTUATORS+2*i)) == 0xff)
      continue;
    hb[2] = i;
    hb[4] = erb(EE_FHT_ACTUATORS+2*i+1);
    for(uint8_t j = 0; j < fht_d3; j++) {
      addParityAndSendData(hb, 5, FHT_CSUM_START, 1);
      my_delay_ms(fht_d4);
    }
    ewb(EE_FHT_ACTUATORS+2*i, 0xa6);
  } while(i--);
  fht8v_timeout = fht_d1;

DH(fht_d1,2); DH(fht_d2,2);
DH(fht_d3,2); DH(fht_d4,2);
DNL();
}



//////////////////////////////////////////////////////////////
uint8_t *
get_fhtbuf(uint8_t *fht)
{
  for(uint8_t i = 0; i < EE_FHTBUF_SIZE; i += 5) {
    if(erb(EE_START_FHTBUF+i  ) != fht[0] ||
       erb(EE_START_FHTBUF+i+1) != fht[1])
      continue;
    return EE_START_FHTBUF+i;
  }
  return 0;
}

//////////////////////////////////////////////////////////////
static void
fht_send(uint8_t *fht, uint8_t msg, uint8_t lb)
{
  uint8_t mbuf[6];

  ccTX();                       // We are deaf
  my_delay_ms(280);             // Sleep for the second msg
  mbuf[0] = fht[0];
  mbuf[1] = fht[1];
  mbuf[2] = msg;
  mbuf[3] = 0x77;
  mbuf[4] = lb;
  addParityAndSendData(mbuf, 5, FHT_CSUM_START, 2);
}

//////////////////////////////////////////////////////////////
static void
fht_send_data(uint8_t *data)
{
  uint8_t mbuf[6];
  ccTX();                       // We are deaf
  my_delay_ms(280);             // Sleep for the second msg
  for(uint8_t i = 0; i < 5; i++)
    mbuf[i] = erb(data+i);
  addParityAndSendData(mbuf, 5, FHT_CSUM_START, 2);
}

// actuator: 0%
// FHZ:can-xmit: 97 *
// can-xmit: 97
// can-rcv: 97
// FHZ:start-xmit: 97
// start-xmit: 97
// FHZ:desired-temp: 20.0
// desired-temp: 20.0
// FHZ:ack: 40
// ack: 40
// end-xmit: 40

void
fht_hook(uint8_t *fht, uint8_t len)
{
  uint8_t *data;

//DC('x');
//DC('s');
//DC('0'+fht80b_xstate);
//DNL();
  if(len != 5)
    return;

  if(fht80b_xstate == XS_IDLE) {

    if(fht[2] == FHT_START_XMIT) {      // The FHT wants to tell us something
      fht_send(fht, FHT_START_XMIT, fht_hc[0]);
      fht80b_xstate = XS_WAITFOR_ACKXMIT;
      return;
    }

    if(!(data = get_fhtbuf(fht)))
      return;

    fht80b_xid[0] = fht[0];
    fht80b_xid[1] = fht[1];
    if(fht[2] == FHT_START_XMIT)
    fht_send(fht, FHT_CAN_XMIT, fht_hc[0]);
    fht80b_xstate = XS_WAITFOR_ACKRCV;
    return;

  } 

  if(fht80b_xid[0] != fht[0] || fht80b_xid[1] != fht[1]) {

    fht80b_xstate = XS_IDLE;
    return;

  }


  if(fht80b_xstate == XS_WAITFOR_ACKRCV) {

    if(fht[2] != FHT_CAN_RCV && fht[2] != FHT_CAN_XMIT)
      fht80b_xstate = XS_IDLE;
    if(fht[2] != FHT_CAN_RCV)
      return;

    fht_send(fht, FHT_START_XMIT, fht_hc[0]);
    fht80b_xstate = XS_WAITFOR_ACKXMIT;
    return;
  }


  if(fht80b_xstate == XS_WAITFOR_ACKXMIT) {
    if(fht[2] != FHT_START_XMIT) {
      fht80b_xstate = XS_IDLE;
      return;
    }
    data = get_fhtbuf(fht);
    fht_send_data(data);
    fht80b_xstate = XS_WAITFOR_ACKDATA;
    return;
  }

  if(fht80b_xstate == XS_WAITFOR_ACKDATA) {

    data = get_fhtbuf(fht);
    if(erb(data+2) != fht[2]) {
      fht80b_xstate = XS_IDLE;
      return;
    }
    ewb(data, 0xff);                                    // Delete old packet

    data = get_fhtbuf(fht);                             // Search for next
    if(!data) {
      fht_send(fht, FHT_ACK, fht[5]);
      fht80b_xstate = XS_IDLE;
    } else {
      fht_send_data(data);
    }

  }


}
