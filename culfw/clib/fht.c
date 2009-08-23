#include <string.h>
#include <avr/eeprom.h>
#include "board.h"
#include "rf_send.h"
#include "rf_receive.h"
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


uint8_t fht_hc0, fht_hc1; // Our housecode. The first byte for 80b communication

#ifdef HAS_FHT_80b
 
       uint8_t fht80b_timeout;
static uint8_t fht80b_state;    // 80b state machine
static uint8_t fht80b_ldata;    // last data waiting for ack
static uint8_t fht80b_out[6];   // Last sent packet. Reserve 1 byte for checksum
static uint8_t fht80b_repeatcnt;
static  int8_t fht80b_minoffset, fht80b_hroffset;
static uint8_t fht80b_buf[FHTBUF_SIZE];
static uint8_t fht80b_bufoff;   // offset in the current fht80b_buf

static void    fht80b_print(void);
static void    fht80b_initbuf(void);
static uint8_t fht_bufspace(void);
static void    fht_delbuf(uint8_t *buf);
static uint8_t fht_addbuf(char *in);
static uint8_t fht_getbuf(uint8_t *buf);
static void    fht80b_reset_state(void);

#endif 

#ifdef HAS_FHT_8v
       uint8_t fht8v_timeout = 0;      
       uint8_t fht8v_buf[18];
static void set_fht8v_timer(void);
#endif

void
fht_init(void)
{
  fht_hc0 = erb(EE_FHTID);
  fht_hc1 = erb(EE_FHTID+1);
#ifdef HAS_FHT_8v
  set_fht8v_timer();
  for(uint8_t i = 0; i < 18; i+=2)
    fht8v_buf[i] = 0xff;
#endif

#ifdef HAS_FHT_80b
  fht80b_reset_state();
  fht80b_initbuf();
#endif
}

void
fhtsend(char *in)
{
  uint8_t hb[6], l;                    // Last byte needed for 8v checksum
  l = fromhex(in+1, hb, 5);

  if(l < 4) {

    if(hb[0] == 1) {                   // Set housecode, clear buffers
      if(l == 3) {
        ewb(EE_FHTID  , hb[1]);        // 1.st byte: 80b relevant
        ewb(EE_FHTID+1, hb[2]);        // 1.st+2.nd byte: 8v relevant
        fht_init();
      } else {
        DH2(fht_hc0);
        DH2(fht_hc1);
        DNL();
      }

#ifdef HAS_FHT_80b
    } else if(hb[0] == 2) {            // Return the 80b buffer
      fht80b_print();

    } else if(hb[0] == 3) {            // Return the remaining fht buffer
      DH2(fht_bufspace());
      DNL();

    } else if(hb[0] == 4) {            // Needed by FHT_MINUTE below
      sec = hb[1];

#endif

#ifdef HAS_FHT_8v
    } else if(hb[0] == 10) {           // Return the 8v buffer

      uint8_t na=0, v, i = 8;
      do {
        if((v = fht8v_buf[2*i]) == 0xff)
          continue;
        if(na)
          DC(' ');
        DH2(i); DC(':');
        DH2(v);
        DH2(fht8v_buf[2*i+1]);
        na++;
      } while(i--);

      if(na==0)
        DS_P( PSTR("N/A") );
      DNL();

    } else if(hb[0] == 11) {           // Return the next 8v timeout
      DU(fht8v_timeout, 2);
      DNL();
#endif
    }

  } else {

#ifdef HAS_FHT_8v
    if(hb[0]==fht_hc0 &&
       hb[1]==fht_hc1) {             // FHT8v mode commands

      if(hb[3] == 0x2f ||              // Pair: Send immediately
         hb[3] == 0x20 ||              // Syncnow: Send immediately
         hb[3] == 0x2c) {              // Sync: Send immediately

        addParityAndSend(in, FHT_CSUM_START, 2);

      } else {                         // Store the rest

        uint8_t a = (hb[2] < 9 ? hb[2] : 0);
        fht8v_buf[a*2  ] = hb[3];      // Command or 0xff for disable
        fht8v_buf[a*2+1] = hb[4];      // value
        set_fht8v_timer();

      }
      return;
    }
#endif

#ifdef HAS_FHT_80b
    if(!fht_addbuf(in))                // FHT80b mode: Queue everything
      DS_P( PSTR("EOB") );
#endif

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

  hb[0] = fht_hc0;
  hb[1] = fht_hc1;

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


//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////
#ifdef HAS_FHT_80b

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

static void
fht80b_sendpacket(void)
{
  ccStrobe(CC1100_SIDLE);               // Don't let the CC1101 to disturb us
                                        // avg. FHT packet is 75ms 
  my_delay_ms(fht80b_out[2]==FHT_CAN_XMIT ? 155 : 75);
  addParityAndSendData(fht80b_out, 5, FHT_CSUM_START, 1);
  ccRX();                               // reception might be lost due to LOVF

  if(tx_report & REP_FHTPROTO) {
    DC('T');
    for(uint8_t i = 0; i < 5; i++)
      DH2(fht80b_out[i]);
    if(tx_report & REP_RSSI)
      DH2(250);
    DNL();
  }
}

static void
fht80b_reset_state(void)
{
  fht80b_state = 0;
  fht80b_ldata = 0;
  fht80b_bufoff = 3;
}

void
fht80b_timer(void)
{
  if(fht80b_repeatcnt) {
    fht80b_sendpacket();
    fht80b_timeout = 41;               // repeat if there is no msg for 0.3sec
    fht80b_repeatcnt--;
  } else {
    fht80b_reset_state();
  }
}

static void
fht80b_send_repeated(void)
{
  fht80b_sendpacket(); 
  if(fht80b_out[2]==FHT_ACK2)
    return;
  fht80b_timeout = 41;
  fht80b_repeatcnt = 1;                // Never got reply for the 3.rd data
}

void
fht_hook(uint8_t *fht_in)
{
  uint8_t fi0 = fht_in[0];             // Makes code 18 bytes smaller...
  uint8_t fi1 = fht_in[1];
  uint8_t fi2 = fht_in[2];
  uint8_t fi3 = fht_in[3];
  uint8_t fi4 = fht_in[4];

  fht80b_timeout = 0;                  // no resend if there is an answer
  if(fht_hc0 == 0 && fht_hc1 == 0)     // FHT processing is off
    return;

  if(fht80b_out[0] != fi0 ||           // A different FHT is sending data
     fht80b_out[1] != fi1) {
    fht80b_reset_state();
    fht80b_out[0] = fi0;
    fht80b_out[1] = fi1;
  }

  if(fht80b_state == FHT_FOREIGN)       // This is not our FHT, forget it
    return;

  uint8_t inb, outb;
  fht80b_out[2] = fi2;                  // copy the in buffer as it cannot
  fht80b_out[3] = fi3;                  // be used for sending it out again
  fht80b_out[4] = fi4;


  //////////////////////////////
  // FHT->CUL part: ack everything.
  if(fi2 && fht80b_state == 0) {            

    if(fi4 != fht_hc0 &&                // Foreign (not our) FHT/FHZ, forget it
       fi4 != 100 &&                    // not the unassigned one
       (fi2 == FHT_CAN_XMIT ||          // FHZ start seq
        fi2 == FHT_CAN_RCV  ||          // FHZ start seq?
        fi2 == FHT_START_XMIT)) {       // FHT start seq
      fht80b_state = FHT_FOREIGN;
      return;
    }

    // Setting fht80b_out[4] to our hosecode for FHT_CAN_RCV & FHT_START_XMIT
    // won't change the FHT pairing
    if(fi2 == FHT_CAN_RCV)
      return;

    if((fi3&0xf0) == 0x60) {            // Ack only 0x6. packets, else we get
      fht80b_out[3] = fi3|0x70;         // strange "endless" loops when other
      fht80b_send_repeated();           // CUL's / FHT's are involved
    }
    return;

  }


  //////////////////////////////
  // CUL->FHT part
  if(!fht_getbuf(fht80b_out) && fht80b_state == 0)
    return;

  // What is the FHT80 supposed to send?
  inb = (fht80b_ldata ? fht80b_ldata :
                                __LPM(fht80b_state_tbl+fht80b_state));

  // We frequently loose the FHT_CAN_XMIT msg from the FHT
  // so skip this state if the next msg comes in.
  if(inb == FHT_CAN_XMIT && fi2 == FHT_CAN_RCV) {
    inb = FHT_CAN_RCV;
    fht80b_state += 2;
  }
    
  // Ack-Check: correct ack from the FHT?
  if(inb != fi2) {
    fht80b_reset_state();
    return;
  }

  outb = 0;                             // What should the CUL send back?

  if(fht80b_ldata) {
    fht80b_bufoff += 2;

    if(fht_getbuf(fht80b_out)) {        // Search for next
      outb = FHT_DATA;

    } else {
      fht80b_state+=2;
    }
  }

  if(!outb)
    outb = __LPM(fht80b_state_tbl+fht80b_state+1);

  if(outb == FHT_DATA) {            

    fht80b_ldata = fht80b_out[2];

    uint8_t f4 = fht80b_out[4];         // makes code 8 bytes smaller
    if(fht80b_ldata == FHT_MINUTE) {
      f4 = (minute + 60 - fht80b_minoffset);
      if(f4 > 60)
        f4 -= 60;
    }
    if(fht80b_ldata == FHT_HOUR) {
      f4 = (hour + 24 - fht80b_hroffset);
      if(f4 > 24)
        f4 -= 24;
    }
    fht80b_out[4] = f4;

  } else {                              // Follow the protocol table
    fht80b_out[2] = outb;
    fht80b_out[3] = 0x77;
    fht80b_out[4] = fht_hc0;
    fht80b_state += 2;
    fht80b_ldata = 0;
    if(fht80b_state == sizeof(fht80b_state_tbl)) {
      // If we delete the buffer earlier, and our conversation aborts, then the
      // FHT will send every 10 minutes from here on a can_rcv telegram, and
      // will stop sending temperature messages
      fht_delbuf(fht80b_out);
      fht80b_reset_state();
    }

  }

  if(outb)
    fht80b_send_repeated();

}

///////////////////////
// FHT buffer management functions
// "Dumb" FHZ model: store every incoming message in a separate buffer.
// Transmit only one buffer at a time.  Delete a complete buffer if a
// transmission is complete.
static uint8_t*
fht_lookbuf(uint8_t *buf)
{
  uint8_t *p = fht80b_buf;
  while(p[0] && p < (fht80b_buf+FHTBUF_SIZE)) {
    if(buf != 0 && p[1] == buf[0] && p[2] == buf[1])
      return p;
    p += p[0];
  }
  return (buf ? 0 : p);
}

static void
fht_delbuf(uint8_t *buf)
{
  uint8_t *p = fht_lookbuf(buf);
  if(p == 0)
    return;

  uint8_t sz = p[0];
  uint8_t *lim = fht80b_buf+FHTBUF_SIZE-sz; 

  while(p < lim) {
    p[0]=p[sz];
    p++;
  }
  p[0] = 0;
}


static uint8_t
fht_addbuf(char *in)
{
  uint8_t *p = fht_lookbuf(0);
  uint8_t l = strlen(in+1)/2+1;
  uint8_t i, j;

  if((p-fht80b_buf) > FHTBUF_SIZE-l)
    return 0;

  p[0] = l;
  for(i=1, j=1 ; j < l; i+=2, j++) {
    fromhex(in+i, p+j, 1);
    if(j > 2 && ((j&1) == 0)) {
      if(p[j-1] == FHT_MINUTE) 
        fht80b_minoffset = minute-p[j];
      if(p[j-1] == FHT_HOUR) 
        fht80b_hroffset = hour-p[j];
    }
  }
  if(p < (fht80b_buf+FHTBUF_SIZE))
    p[j] = 0;
  return 1;
}

static uint8_t
fht_bufspace(void)
{
  return (FHTBUF_SIZE - (uint8_t)(fht_lookbuf(0)-fht80b_buf));
}

static uint8_t
fht_getbuf(uint8_t *buf)
{
  uint8_t *p = fht_lookbuf(buf);
  if(p == 0 || p[0] <= fht80b_bufoff)
    return 0;
  buf[2] = p[fht80b_bufoff];
  buf[3] = 0x79;
  buf[4] = p[fht80b_bufoff+1];
  return 1;
}

static void
fht80b_initbuf()
{
  fht80b_buf[0] = 0;
}

static void
fht80b_print()
{
  uint8_t *p = fht80b_buf;

  if(!p[0])
    DS_P( PSTR("N/A") );
  while(p[0] && (p < (fht80b_buf+FHTBUF_SIZE))) {
    if(p != fht80b_buf)
      DC(' ');
    uint8_t i = 1;
    DH2(p[i++])
    DH2(p[i++])
    DC(':');
    while(i < p[0]) {
      if(i > 3 && (i&1))
        DC(',');
      DH2(p[i++]);
    }
    p += p[0];
  }
  DNL();
}

#endif
