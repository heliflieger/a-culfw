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
uint8_t fht80b_state;    // 80b state machine
uint8_t fht80b_ldata;    // last data waiting for ack
uint8_t fht80b_timeout;
 int8_t fht80b_minoffset;
uint8_t fht80b_buf[FHTBUF_SIZE];

static void     fht80b_print(void);
static void     fht80b_initbuf(void);
static uint8_t  fht_free(void);
static void fht_delbuf(uint8_t *buf);
static uint8_t  fht_addbuf(uint8_t *buf);
static uint8_t  fht_getbuf(uint8_t *buf);

#endif 

#ifdef HAS_FHT_8v
uint8_t fht8v_timeout = 0;      
static void set_fht8v_timer(void);
uint8_t fht8v_buf[18];
#endif

void
fht_init(void)
{
  fht_hc[0] = erb(EE_FHTID);
  fht_hc[1] = erb(EE_FHTID+1);
#ifdef HAS_FHT_8v
  set_fht8v_timer();
  for(uint8_t i = 0; i < 18; i+=2)
    fht8v_buf[i] = 0xff;
#endif

#ifdef HAS_FHT_80b
  fht80b_state = 0;
  fht80b_ldata = 0;
  fht80b_initbuf();
#endif
}

void
fhtsend(char *in)
{
  uint8_t hb[5], l;

  l = fromhex(in+1, hb, 6);

  if(l == 3 && hb[0] == 1) {                // Set housecode, clear buffers
    if(fht_hc[0] != hb[1] && fht_hc[1] != hb[2]) {
      ewb(EE_FHTID  , hb[1]);               // 1.st byte: 80b relevant
      ewb(EE_FHTID+1, hb[2]);               // 1.st+2.nd byte: 8v relevant
    }
    fht_init();

  } else if(l == 1 && hb[0] == 1) {         // Report own housecode: 
    DH(fht_hc[0], 2);
    DH(fht_hc[1], 2);
    DNL();

#ifdef HAS_FHT_80b
  } else if(l == 1 && hb[0] == 2) {         // Return the 80b buffer
    fht80b_print();

  } else if(l == 1 && hb[0] == 3) {         // Return the remaining fht buffer
    DH(fht_free(), 2);
    DNL();

  } else if(l == 2 && hb[0] == 4) {         // Needed by FHT_MINUTE below
    sec = hb[1];
#endif

#ifdef HAS_FHT_8v
  } else  if(l == 1 && hb[0] == 10) {        // Return the 8v buffer

    uint8_t na=0, v, i = 8;
    do {
      if((v = fht8v_buf[2*i]) == 0xff)
        continue;
      if(na)
        DC(' ');
      DH(i, 2); DC(':');
      DH(v,2);
      DH(fht8v_buf[2*i+1],2);
      na++;
    } while(i--);

    if(na==0)
      DS_P( PSTR("N/A") );
    DNL();

  } else if(l == 1 && hb[0] == 11) {         // Return the next 8v timeout
    DU(fht8v_timeout, 2);
    DNL();
#endif

  } else if(l == 5) {

#ifdef HAS_FHT_8v
    if(hb[0]==fht_hc[0] &&
       hb[1]==fht_hc[1]) {             // FHT8v mode commands

      if(hb[3] == 0x2f ||                     // Pair: Send immediately
         hb[3] == 0x20 ||                     // Syncnow: Send immediately
         hb[3] == 0x2c) {                     // Sync: Send immediately

        addParityAndSend(in, FHT_CSUM_START, 2);

      } else {                                // Store the rest

        uint8_t a = (hb[2] < 9 ? hb[2] : 0);
        fht8v_buf[a*2  ] = hb[3];    // Command or 0xff for disable
        fht8v_buf[a*2+1] = hb[4];    // value
        set_fht8v_timer();

      }

    } else
#endif

#ifdef HAS_FHT_80b
    {                                  // FHT80b mode: Queue everything

#define FHT_MINUTE     0x64
      // Look for an empty slot. Note: it should be a distinct ringbuffer for
      // each controlled fht
      if(!fht_addbuf(hb))
        DS_P( PSTR("EOB") );
      if(hb[2] == FHT_MINUTE)
        fht80b_minoffset = minute-hb[4];

    }
#else
      ;
#endif

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
    if(fht8v_buf[2*i] != 0xff)
      break;
  if(i == 9)
    fht8v_timeout = 0;
  else if(fht8v_timeout == 0)
    fht8v_timeout = 116;
}

void
fht8v_timer(void)
{
  uint8_t hb[6], i = 8;

  hb[0] = fht_hc[0];
  hb[1] = fht_hc[1];

  do {
    if((hb[3] = fht8v_buf[2*i]) == 0xff)
      continue;
    hb[2] = i;
    hb[4] = fht8v_buf[2*i+1];
    for(uint8_t j = 0; j < FHT_NMSG; j++) {
      addParityAndSendData(hb, 5, FHT_CSUM_START, 1);
      my_delay_ms(64);
    }
    fht8v_buf[2*i] = 0xa6;
  } while(i--);
  fht8v_timeout = 116;

}
#endif


#ifdef HAS_FHT_80b

#define FHT_ACTUATOR   0x00
#define FHT_ACK        0x4B
#define FHT_CAN_XMIT   0x53
#define FHT_CAN_RCV    0x54
#define FHT_ACK2       0x69
#define FHT_START_XMIT 0x7D
#define FHT_END_XMIT   0x7E

#define FHT_DATA       0xff
#define FHT_FOREIGN    0xff

PROGMEM prog_uint8_t fht80b_state_tbl[] = {
  //FHT80b          //CUL, 0 for no answer
  FHT_ACTUATOR,     FHT_CAN_XMIT,       // We are deaf for the second ACTUATOR
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
//////////////////////////////////////////////////////////////
static void
fht_sendpacket(uint8_t *mbuf)
{
  ccStrobe( CC1100_SIDLE );    // Don't let the CC1101 to disturb us
  if(mbuf[2]==FHT_CAN_XMIT)    // avg. FHT packet is 76ms 
    my_delay_ms(80);           // Make us deaf for the second ACTUATOR packet.
  addParityAndSendData(mbuf, 5, FHT_CSUM_START, FHT_NMSG);
  ccRX();                      // reception might be lost due to LOVF
  DC('T'); for(uint8_t i = 0; i < 5; i++) DH(mbuf[i],2); DH(0,2); DNL();
}

void
fht80b_timer(void)
{
  fht80b_state = 0;
}

void
fht_hook(uint8_t *fht_in)
{
  if(fht_hc[0] == 0 && fht_hc[1] == 0)  // FHT processing is off
    goto DONE;

  if(fht80b_last[0] != fht_in[0] ||     // A different FHT is sending data
     fht80b_last[1] != fht_in[1]) {
    fht80b_state = 0;
    fht80b_ldata = 0;
    fht80b_last[0] = fht_in[0];
    fht80b_last[1] = fht_in[1];
  }

  if(fht80b_state == FHT_FOREIGN)       // This is not our FHT, forget it
    goto DONE;

  uint8_t fht_out[6], inb, outb;        // Reserve 1 byte for checksum
  for(uint8_t i = 0; i < 5; i++)            // Copy the input, as it
    fht_out[i] = fht_in[i];             // cannot be reused for sending

  //////////////////////////////
  // FHT->CUL part: ack everything.
  if(fht_in[2] && fht80b_state == 0) {            

    if(fht_in[4] != fht_hc[0] &&        // Foreign (not our) FHT/FHZ, forget it
       fht_in[4] != 100 &&              // not the unassigned one
       (fht_in[2] == FHT_CAN_XMIT ||    // FHZ start seq
        fht_in[2] == FHT_CAN_RCV  ||    // FHZ start seq?
        fht_in[2] == FHT_START_XMIT)) { // FHT start seq
      fht80b_state = FHT_FOREIGN;
      goto DONE;
    }

    // Setting fht_out[4] to our hosecode for FHT_CAN_RCV & FHT_START_XMIT
    // won't change the FHT pairing
    if(fht_in[2] == FHT_CAN_RCV) {
      if(fht_getbuf(fht_out)) {
        fht80b_state = RCV_OFFSET;
        goto FHT_CONVERSATION;
      }
      fht_out[2] = FHT_END_XMIT;        // Send "Shut up" message
    }

    if((fht_in[3]&0xf0) == 0x60) {      // Answer only 0x6. packets, else we get
      fht_out[3] |= 0x70;               // strange "endless" loops when other
      fht_sendpacket(fht_out);          // CUL's / FHT's are involved
    }
    goto DONE;

  }

  //////////////////////////////
  // CUL->FHT part
  if(!fht_getbuf(fht_out) && fht80b_state == 0)
    goto DONE;

FHT_CONVERSATION:
  // What is the FHT80 supposed to send?
  inb = (fht80b_ldata ? fht80b_ldata :
                                __LPM(fht80b_state_tbl+fht80b_state));

  // We frequently loose the FHT_CAN_XMIT msg from the FHT
  // so skip this state if the next msg comes in.
  if(inb == FHT_CAN_XMIT && fht_in[2] == FHT_CAN_RCV) {
    inb = FHT_CAN_RCV;
    fht80b_state += 2;
  }
    
  // Ack-Check: correct ack from the FHT?
  if(inb != fht_in[2]) {
    fht80b_state = fht80b_ldata = 0;
    goto DONE;
  }

  // What should the CUL send back?
  outb = 0;

  if(fht80b_ldata) {
    fht_delbuf(fht_out);
    if(fht_getbuf(fht_out))       // Search for next
      outb = FHT_DATA;
    else
      fht80b_state+=2;
  }

  if(!outb)
    outb = __LPM(fht80b_state_tbl+fht80b_state+1);

  if(outb == FHT_DATA) {            

    fht80b_ldata = fht_out[2];
    if(fht80b_ldata == FHT_MINUTE) {
      fht_out[4] = (minute + 60 - fht80b_minoffset);
      if(fht_out[4] > 60)
        fht_out[4] -= 60;
    }

  } else {                            // Follow the protocol table

    fht_out[2] = outb;
    fht_out[3] = 0x77;
    fht_out[4] = fht_hc[0];
    fht80b_ldata = 0;
    fht80b_state += 2;
    if(fht80b_state == sizeof(fht80b_state_tbl))
      fht80b_state = 0;

  }

  if(outb) {
    fht_sendpacket(fht_out);
    fht80b_timeout = 125;       // reset the state if there is no msg for 1 sec
  }

DONE:
  ;
}

// MODEL1: A slot contains all messages for a given housecode. Byte 0 is
//         slotlen, byte 1&2 is housecode, followed by 2 byte netto data
//         for each message. There are no empty slots, data is always moved
//         so that all free space is at the end.
//
// MODEL2: 4 byte fixed slots per message. Byte 0&1 is housecode, 2&3 is netto
//         data. Unused slots are marked with byte 0 = 0xff.
//         Uses more RAM then MODEL1 if there is more than 1 message per FHT,
//         else less. Needs 192b less program space.


///////////////////////
// FHT buffer management functions

#ifdef FHTBUF_MODEL1
static uint8_t*
fht_lookbuf(uint8_t *buf)
{
  uint8_t *p = fht80b_buf;
  while(p[0] && p < (fht80b_buf+FHTBUF_SIZE)) {
    if(p[1] == buf[0] && p[2] == buf[1])
      break;
    p += *p;
  }
  return p;
}

static void
fht_delbuf(uint8_t *buf)
{
  uint8_t *p = fht_lookbuf(buf);
  if(p[1] != buf[0] || p[2] != buf[1])
    return;
  uint8_t *p1, *p2, sz;
  if(p[0] == 5) {
    sz = 5; p1 = p;
  } else {
    sz = 2; p1 = p+3;
    p[0] -= 2;
  }
  p2 = fht80b_buf+FHTBUF_SIZE-sz;
  while(p1 < p2) {
    p1[0]=p1[sz];
    p1++;
  }
  while(sz--)
    p1[sz] = 0;
}


static uint8_t
fht_addbuf(uint8_t *buf)
{
  uint8_t *p = fht_lookbuf(buf);

  if(p[1] == buf[0] && p[2] == buf[1]) { // Append only the value
    if(fht_free() < 2)
      return 0;

    // insert 2 bytes
    uint8_t *p1 = p+p[0];
    uint8_t *p2 = fht80b_buf+FHTBUF_SIZE-3;
    while(p2 >= p1) {
      p2[2]=p2[0];
      p2--;
    }

    uint8_t idx = p[0];
    p[idx  ] = buf[2];
    p[idx+1] = buf[4];
    p[0] = idx+2;
  } else {
    if((p-fht80b_buf) > FHTBUF_SIZE-5)
      return 0;
    p[0] = 5;
    p[1] = buf[0];
    p[2] = buf[1];
    p[3] = buf[2];
    p[4] = buf[4];
  }
  return 1;
}

static uint8_t
fht_free(void)
{
  uint8_t buf[] = {0xff,0xff};
  return (FHTBUF_SIZE - (uint8_t)(fht_lookbuf(buf)-fht80b_buf));
}

static uint8_t
fht_getbuf(uint8_t *buf)
{
  uint8_t *p = fht_lookbuf(buf);
  if(p[1] != buf[0] || p[2] != buf[1])
    return 0;
  buf[2] = p[3];
  buf[3] = 0x79;
  buf[4] = p[4];
  return 1;
}

static void
fht80b_initbuf()
{
  uint8_t i = 0;
  while(i < FHTBUF_SIZE)
    fht80b_buf[i++] = 0;
}

static void
fht80b_print()
{
  uint8_t *p = fht80b_buf;
  uint8_t i;

  if(!p[0])
    DS_P( PSTR("N/A") );
  while(p[0]) {
    DH(p[1], 2);
    DH(p[2], 2);
    DC(':');
    for(i = 3; i < p[0]; i+= 2) {
      if(i > 3)
        DC(',');
      DH(p[i], 2);
      DH(p[i+1], 2);
    }
    p += p[0];
  }
  DNL();
}
#endif

///////////////////////////////////////////////////////////////////////
#ifdef FHTBUF_MODEL2

static void
fht_delbuf(uint8_t *buf)
{
  uint8_t i;
  for(i = 0; i < FHTBUF_SIZE; i += 4)
    if(fht80b_buf[i] == buf[0] && fht80b_buf[i+1] == buf[1]) {
      fht80b_buf[i] = 0xff;
      return;
    }
}

static uint8_t
fht_addbuf(uint8_t *buf)
{
  uint8_t i;
  for(i = 0; i < FHTBUF_SIZE; i += 4)
    if(fht80b_buf[i] == 0xff) {
      fht80b_buf[i+0] = buf[0];
      fht80b_buf[i+1] = buf[1];
      fht80b_buf[i+2] = buf[2];
      fht80b_buf[i+3] = buf[4];
      return 1;
    }
  return 0;
}

static uint8_t
fht_free(void)
{
  uint8_t i, j = 0;
  for(i = 0; i < FHTBUF_SIZE; i += 4)
    if(fht80b_buf[i] == 0xff)
      j += 4;
  return j;
}

static uint8_t
fht_getbuf(uint8_t *buf)
{
  uint8_t i;
  for(i = 0; i < FHTBUF_SIZE; i += 4)
    if(fht80b_buf[i] == buf[0] && fht80b_buf[i+1] == buf[1]) {
      buf[2] = fht80b_buf[i+2];
      buf[3] = 0x79;
      buf[4] = fht80b_buf[i+3];
      return 1;
    }
  return 0;
}

static void
fht80b_initbuf()
{
  for(uint8_t i = 0; i < FHTBUF_SIZE; i++)
    fht80b_buf[i] = 0xff;
}

static void
fht80b_print()
{
  uint8_t i, j = 0;
  for(i = 0; i < FHTBUF_SIZE; i += 4)
    if(fht80b_buf[i] != 0xff) {
      if(j)
        DC(' ');
      DH(fht80b_buf[i+0], 2);
      DH(fht80b_buf[i+1], 2);
      DH(fht80b_buf[i+2], 2);
      DH(fht80b_buf[i+3], 2);
      j++;
    }
  if(!j)
    DS_P( PSTR("N/A") );
  DNL();
}
#endif

#endif
