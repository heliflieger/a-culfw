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


#define FHT_CSUM_START          12

uint8_t fht_hc[2];    // Our housecode. The first byte for 80b communication
uint8_t fht_last[2]; 
uint8_t fht_foreign;
uint8_t fht80b_state; // 80b state machine
uint8_t fht8v_timeout = 0xff;      


uint8_t fht_d1 = 116,  // seconds
        fht_d2 = 0xa5, // Delay before
        fht_d3 = 1,    // repetitions
        fht_d4 = 0x40; // delay after

#ifdef HAS_FHT_8v

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
  fht_foreign = 0;
}

void
fht_eeprom_reset(void)
{
  for(uint8_t i = 0; i < 18; i+=2)
    ewb(EE_FHT_ACTUATORS+i, 0xff);
  for(uint8_t i = 0; i < EE_FHTBUF_SIZE; i++)
    ewb(EE_START_FHTBUF+i, 0xff);
  ewb(EE_FHTID,   0);
  ewb(EE_FHTID+1, 0);
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
      fht_eeprom_reset();
      ewb(EE_FHTID  , fht_hc[0]);
      ewb(EE_FHTID+1, fht_hc[1]);

      fht8v_timeout = 0xff;
      fht80b_state = 0;
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

  } else if(hb[0]==fht_hc[0] && hb[1]==fht_hc[1]) {  // FHT8v mode commands

    if(hb[3] == 0x2f ||                        // Pair: Send immediately
       hb[3] == 0x20 ||                        // Syncnow: Send immediately
       hb[3] == 0x2c) {                        // Sync: Send immediately

      addParityAndSend(in, FHT_CSUM_START, 2);

    } else {                                  // Store the rest

      uint8_t a = (hb[2] < 9 ? hb[2] : 0);
      ewb(EE_FHT_ACTUATORS+a*2,  hb[3]);      // Command or 0xff for disable
      ewb(EE_FHT_ACTUATORS+a*2+1,hb[4]);      // value
#ifdef HAS_FHT_8v
      set_fht8v_timer();
#endif

    }

  } else {                                    // FHT80b mode: Queue everything

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
    fht8v_timeout = fht_d1;
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

//DH(fht_d1,2); DH(fht_d2,2);
//DH(fht_d3,2); DH(fht_d4,2);
//DNL();
}
#endif


#ifdef HAS_FHT_80b
#ifdef COMMENT

missed    fl actuator: 0%            T510200B600           .140
4.747 FHT fl actuator: 0%            T510200B600 -66.5     .140
4.887 FHT fl FHZ:can-xmit: 97        T5102537761 -72       .131
5.018 FHT fl can-xmit: 97            T5102536761 -66.5     .114
5.132 FHT fl can-rcv: 97             T5102546761 -66.5     .140
missed    fl FHZ:start-xmit: 97      T51027D7761           .140
5.416 FHT fl start-xmit: 97          T51027D6761 -66.5     .155
5.571 FHT fl FHZ:desired-temp: 19.5  T5102417927 -73.5     .125
5.696 FHT fl desired-temp: 19.5      T5102416927 -66.5     .156
5.852 FHT fl FHZ:ack: 39             T51024B7727 -71       .127
5.979 FHT fl ack: 39                 T51024B6727 -66.5     .159
6.138 FHT fl FHZ:end-xmit: 39        T51027E7727 -72       .126
6.264 FHT fl end-xmit: 39            T51027E6727 -66           

T1234002F00
TFFFF070102
TFFFF08001e
TFFFF011234
TFFFF016161
TFFFF02
T5102417927
TFFFF04

#endif

#define FHT_ACTUATOR   0x00
#define FHT_CAN_XMIT   0x53
#define FHT_CAN_RCV    0x54
#define FHT_ACK        0x4B
#define FHT_START_XMIT 0x7D
#define FHT_END_XMIT   0x7E

#define FHT_ANSWER     0xf4  // 11110100: bitfield of states, when to answer
#define FHT_DATA       5

static uint8_t
fht_state_sequence[] = { 
  FHT_ACTUATOR,
  FHT_ACTUATOR,
  FHT_CAN_XMIT,
  FHT_CAN_RCV,
  FHT_START_XMIT,
  0,
  FHT_ACK,
  FHT_END_XMIT,
};
#define FHT_MAXSTATE          (sizeof(fht_state_sequence)-1)

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
  my_delay_ms(fht_d4);
  DC(':'); for(int i = 0; i < 5; i++) DH(mbuf[i],2); DNL();
  addParityAndSendData(mbuf, 5, FHT_CSUM_START, fht_d3);
}


void
fht_hook(uint8_t *fht)
{
  static uint8_t in_hook = 0;

  if(in_hook)
    return;

  in_hook = 1;

  if(fht_hc[0] == 0 && fht_hc[1] == 0)  // FHT processing is off
    goto DONE;

  if(fht_last[0] != fht[0] || fht_last[1] != fht[1]) {
    fht80b_state = 0;
    fht_foreign = 0;
  }

  fht_last[0] = fht[0];
  fht_last[1] = fht[1];
  if(fht_foreign)
    goto DONE;

  uint8_t fhtb[6];
  for(int i = 0; i < 5; i++)
    fhtb[i] = fht[i];
  fhtb[5] = 0;
    

  //////////////////////////////
  // FHT->CUL part: ack everything.
  if(fht[2] && fht80b_state == 0) {            

    // unassigned FHT's have 100 as housecode but answering to this won't help,
    // and the FHZ does not answer it either
    if(fht[2]==FHT_START_XMIT && fht[4]!=fht_hc[0]) { // Foreign FHT, forget it
      fht_foreign = 1;
      goto DONE;
    }
    fhtb[3] |= 0x70;    // Answer
    fht_sendpacket(fhtb);
    goto DONE;

  }

  uint8_t *data;
  //////////////////////////////
  // CUL->FHT part
  if(!(data = get_fhtbuf(fht)) && fht80b_state == 0)
    goto DONE;
  DC('f'+fht80b_state);
  DH(sec,2);
  DH(hsec,2);

  // Ack-Check: correct ack from the FHT?
  if(fht_state_sequence[fht80b_state] != fht[2]) {
    DC('a'); DNL();
    fht80b_state = 0;
    goto DONE;
  }

  if(fht80b_state == FHT_DATA) {

    ewb(data, 0xff);                                    // Delete old packet
    data = get_fhtbuf(fht);                             // Search for next
    if(!data)
      fht80b_state++;

  } else {

    fht80b_state++;

  }
   
  if(fht80b_state == FHT_DATA) {

    fhtb[2] = erb(data+2);        
    fhtb[3] = erb(data+3);
    fhtb[4] = erb(data+4);
    fht_sendpacket(fhtb);
    fht_state_sequence[FHT_DATA] = fhtb[2]; // for Ack-Check

  } else {

    if(fht80b_state == FHT_MAXSTATE) {          // EOM
      DC('E'); DNL();
      fht80b_state = 0;
      goto DONE;
    }
      
    if((1<<fht80b_state) & FHT_ANSWER) {
      fhtb[2] = fht_state_sequence[fht80b_state];
      fhtb[3] = 0x77;
      fhtb[4] = fht_hc[0];
      fht_sendpacket(fhtb);
    }

  }
  DH(sec,2);
  DH(hsec,2);
  DC('x'); DNL();
DONE:
  in_hook = 0;
}
#endif
