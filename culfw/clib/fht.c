#include <avr/eeprom.h>
#include "board.h"
#include "transceiver.h"
#include "fncollection.h"
#include "display.h"
#include "fht.h"
#include "delay.h"
#include "clock.h"
#include "cc1100.h"


// We have two different work models:
// 1. Control the FHT80b. In this mode we have to wait for a message from the
//    FHT80b, and then send all buffered messages to it, with a strange
//    ask/reply protocol.
// 2. Control the valves directly (8v mode). In this mode we have to send every
//    116 second a message to the valves. Some messages (e.g. pair) are sent
//    directly. This mode is activated when we receive an actuator message for
//    our own housecode.


#define FHT_CSUM_START   12
#define FHT_NMSG          2

uint8_t fht_hc[2];    // Our housecode. The first byte for 80b communication

#ifdef HAS_FHT_80b
uint8_t fht80b_last[2]; 
uint8_t fht80b_foreign;  // Not our fht
uint8_t fht80b_state;    // 80b state machine
uint8_t fht80b_ldata;    // last data waiting for ack
#endif 

#ifdef HAS_FHT_8v
uint8_t fht8v_timeout = 0xff;      
static void set_fht8v_timer(void);
#endif

void
fht_init(void)
{
  fht_hc[0] = erb(EE_FHTID);
  fht_hc[1] = erb(EE_FHTID+1);
#ifdef HAS_FHT_8v
  set_fht8v_timer();
#endif
  fht80b_state = 0;
  fht80b_ldata = 0;
  fht80b_foreign = 0;
}

void
fht_eeprom_reset(uint8_t all)
{
  for(uint8_t i = 0; i < 18; i+=2)
    ewb(EE_FHT_ACTUATORS+i, 0xff);
  for(uint8_t i = 0; i < EE_FHTBUF_SIZE; i++)
    ewb(EE_START_FHTBUF+i, 0xff);
  if(all) {
    ewb(EE_FHTID,   0);
    ewb(EE_FHTID+1, 0);
  }
}

void
fhtsend(char *in)
{
  uint8_t hb[5], l;

  l = fromhex(in+1, hb, 6);

  if(l == 3 && hb[0] == 1) {                // Set housecode, clear buffers
    fht_eeprom_reset(0);
    ewb(EE_FHTID  , hb[1]);                 // 1.st byte: 80b relevant
    ewb(EE_FHTID+1, hb[2]);                 // 1.st+2.nd byte: 8v relevant
    fht_init();

  } else if(l == 1 && hb[0] == 1) {         // Report own housecode: 
    DH(fht_hc[0], 2);
    DH(fht_hc[1], 2);
    DNL();

  } else if(l == 1 && hb[0] == 2) {         // Return the 80b buffer
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

#ifdef HAS_FHT_8v
  } else  if(l == 1 && hb[0] == 3) {        // Return the 8v buffer
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

  } else if(l == 1 && hb[0] == 4) {         // Return the next 8v timeout
    DU(fht8v_timeout, 2);
    DNL();
#endif

  } else if(l == 5) {

    if(hb[0]==fht_hc[0] &&
       hb[1]==fht_hc[1]) {             // FHT8v mode commands

      if(hb[3] == 0x2f ||                     // Pair: Send immediately
         hb[3] == 0x20 ||                     // Syncnow: Send immediately
         hb[3] == 0x2c) {                     // Sync: Send immediately

        addParityAndSend(in, FHT_CSUM_START, 2);

      } else {                                // Store the rest

        uint8_t a = (hb[2] < 9 ? hb[2] : 0);
        ewb(EE_FHT_ACTUATORS+a*2,  hb[3]);    // Command or 0xff for disable
        ewb(EE_FHT_ACTUATORS+a*2+1,hb[4]);    // value
  #ifdef HAS_FHT_8v
        set_fht8v_timer();
  #endif

      }

    } else {                                  // FHT80b mode: Queue everything

      // Look for an empty slot. Note: it should be a distinct ringbuffer for
      // each controlled fht
      uint8_t i;
      for(i = 0; i < EE_FHTBUF_SIZE; i += 5) {
        if(erb(EE_START_FHTBUF+i) != 0xff)
          continue;
        for(uint8_t j = 0; j < 5; j++) {
          ewb(EE_START_FHTBUF+i+j, hb[j]);
        }
        break;
      }
      if(i == EE_FHTBUF_SIZE)
        DS_P( PSTR("EOB") );

    }

  } else {
  
    DC('?'); DNL();

  }

}

#ifdef HAS_FHT_8v
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
    fht8v_timeout = 116;
}

void
fht8v_timer(void)
{
  uint8_t hb[6], i = 8;

  hb[0] = fht_hc[0];
  hb[1] = fht_hc[1];

  do {
    if((hb[3] = erb(EE_FHT_ACTUATORS+2*i)) == 0xff)
      continue;
    hb[2] = i;
    hb[4] = erb(EE_FHT_ACTUATORS+2*i+1);
    for(uint8_t j = 0; j < FHT_NMSG; j++) {
      addParityAndSendData(hb, 5, FHT_CSUM_START, 1);
      my_delay_ms(64);
    }
    ewb(EE_FHT_ACTUATORS+2*i, 0xa6);
  } while(i--);
  fht8v_timeout = 116;

}
#endif


#ifdef HAS_FHT_80b

#define FHT_ACTUATOR   0x00
#define FHT_CAN_XMIT   0x53
#define FHT_CAN_RCV    0x54
#define FHT_ACK        0x4B
#define FHT_START_XMIT 0x7D
#define FHT_END_XMIT   0x7E
#define FHT_ACK2       0x69
#define FHT_DATA       0xff

/*
48.711      actuator: 0%
48.780 .069 actuator: 0%
48.921 .141 FHZ:can-xmit: 97
49.052 .131 can-xmit: 97
49.166 .114 can-rcv: 97
49.324 .158 FHZ:start-xmit: 97
49.430 .106 can-rcv: 97
49.665 .235 FHZ:start-xmit: 97
49.794 .129 start-xmit: 97
49.950 .156 FHZ:desired-temp: 18.0
50.075 .125 desired-temp: 18.0
50.231 .156 FHZ:ack: 36
50.357 .126 ack: 36
50.514 .157 FHZ:end-xmit: 36
50.640 .126 end-xmit: 36
*/


PROGMEM prog_uint8_t fht80b_state_tbl[] = {
  //FHT80b          //CUL, 0 for no answer
  FHT_ACTUATOR,     FHT_CAN_XMIT,
  FHT_CAN_XMIT,     0,
  FHT_CAN_RCV,      FHT_START_XMIT,
  FHT_START_XMIT,   FHT_DATA,
  FHT_DATA,         FHT_ACK,
  FHT_ACK,          FHT_END_XMIT,
  FHT_END_XMIT,     0
};
#define RCV_OFFSET 6

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////
// CUL <-> FHT Communication
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
fht_sendpacket(uint8_t *mbuf)
{
  ccStrobe( CC1100_SIDLE );    // Don't let the CC1101 to disturb us
  if(mbuf[2]==FHT_CAN_XMIT)    // avg. FHT packet is 76ms 
    my_delay_ms(80);           // Make us deaf for the second ACTUATOR packet.
  addParityAndSendData(mbuf, 5, FHT_CSUM_START, FHT_NMSG);
  DC('T'); for(int i = 0; i < 5; i++) DH(mbuf[i],2); DH(0,2); DNL();
}


void
fht_hook(uint8_t *fht)
{
  if(fht_hc[0] == 0 && fht_hc[1] == 0)  // FHT processing is off
    goto DONE;

  if(fht80b_last[0] != fht[0] ||        // A different FHT is sending data
     fht80b_last[1] != fht[1]) {
    fht80b_state = 0;
    fht80b_ldata = 0;
    fht80b_foreign = 0;
  }

  fht80b_last[0] = fht[0];
  fht80b_last[1] = fht[1];
  if(fht80b_foreign)                    // This is not our FHT, forget it
    goto DONE;

  uint8_t fhtb[6];
  for(int i = 0; i < 5; i++)            // Copy the input, as it
    fhtb[i] = fht[i];                   // cannot be reused for sending
  fhtb[5] = 0;
    
  uint8_t *data, inb, outb;

  //////////////////////////////
  // FHT->CUL part: ack everything.
  if(fht[2] && fht80b_state == 0) {            

    if(fht[4] != fht_hc[0] &&           // Foreign (not our) FHT/FHZ, forget it
       fht[4] != 100 &&                 // not the unassigned one
       (fht[2] == FHT_CAN_XMIT ||       // FHZ start seq
        fht[2] == FHT_CAN_RCV  ||       // FHZ start seq?
        fht[2] == FHT_START_XMIT)) {    // FHT start seq
      fht80b_foreign = 1;
      goto DONE;
    }

    if(fht[2] == FHT_CAN_RCV) {
      if((data = get_fhtbuf(fht))) {
        fht80b_state = RCV_OFFSET;
        goto FHT_CONVERSATION;
      }
      fhtb[2] = FHT_END_XMIT;         // Send "Shut up" message
      fhtb[4] = fht_hc[0];
    }

    if((fhtb[3]&0xf0) == 0x60) {        // Answer only 0x6. packets, else we get
      fhtb[3] |= 0x70;                  // strange "endless" loops when other
      fht_sendpacket(fhtb);             // CUL's / FHT's are involved
    }
    goto DONE;

  }

  //////////////////////////////
  // CUL->FHT part
  if(!(data = get_fhtbuf(fht)) && fht80b_state == 0)
    goto DONE;

FHT_CONVERSATION:
  // What is the FHT80 supposed to send?
  inb = (fht80b_ldata ? fht80b_ldata :
                                __LPM(fht80b_state_tbl+fht80b_state));

  // We frequently loose the FHT_CAN_XMIT msg from the FHT
  if(inb == FHT_CAN_XMIT && fht[2] == FHT_CAN_RCV) {
    inb = FHT_CAN_RCV;
    fht80b_state += 2;
  }
    
  // Ack-Check: correct ack from the FHT?
  if(inb != fht[2]) {
    fht80b_state = fht80b_ldata = 0;
    goto DONE;
  }

  // What should the CUL send back?
  outb = 0;

  if(fht80b_ldata) {
    ewb(data, 0xff);                   // Delete old packet
    if((data = get_fhtbuf(fht)))       // Search for next
      outb = FHT_DATA;
    else
      fht80b_state+=2;
  }

  if(!outb)
    outb = __LPM(fht80b_state_tbl+fht80b_state+1);

  if(outb == FHT_DATA) {            

    fhtb[2] = erb(data+2);        
    fhtb[3] = erb(data+3);
    fhtb[4] = erb(data+4);
    fht80b_ldata = fhtb[2];

  } else {                            // Follow the protocol table

    fhtb[2] = outb;
    fhtb[3] = 0x77;
    fhtb[4] = fht_hc[0];
    fht80b_ldata = 0;
    fht80b_state += 2;
    if(fht80b_state == sizeof(fht80b_state_tbl))
      fht80b_state = 0;

  }

  if(outb)
    fht_sendpacket(fhtb);

DONE:
  ;
}
#endif
