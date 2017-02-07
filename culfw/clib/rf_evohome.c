/*****************************************************************************/
/*                                                                           */
/* Honeywell Evohome/Evotouch/Ramses support                                 */
/*                                                                           */
/* This module builds on infomation gleaned from many sources, but was       */
/* written by Colin Tregenza Dancer                                          */
/*                                                                           */
/*****************************************************************************/

#include "board.h"
#ifdef HAS_EVOHOME
#include <string.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include "delay.h"
#include "display.h"
#include "clock.h"
#include "rf_evohome.h"
#include "cc1100.h"

/*****************************************************************************/
/* Manchester encode - Needing just 16 bytes, the encode array is a worth    */
/* using.  Caller's responsibility to only pass nibbles, but anding for      */
/* safety.                                                                   */
/*****************************************************************************/
#define MANCHESTER_ENCODE(X) (manchester_encode[(0xF&((uint8_t)(X)))])
uint8_t manchester_encode[16]={0xAA,0xA9,0xA6,0xA5,0x9A,0x99,0x96,0x95,0x6A,0x69,0x66,0x65,0x5A,0x59,0x56,0x55};

/*****************************************************************************/
/* Memory compact manchester decode                                          */
/*                                                                           */
/* Used XOR for fast validity test                                           */
/*                                                                           */
/* Return: valid data => 0x0-0xF, invalid=0xFF                               */
/*****************************************************************************/
#define MANCHESTER_DECODE(X) manchester_decode(X)
uint8_t manchester_decode(uint8_t data);
uint8_t manchester_decode(uint8_t data)
{
  return((((data&0x55)^((data&0xAA)>>1)) == 0x55) ? ((((data & 0x06)>>1) | ((data & 0x60)>>3)) ^ 0x05) : 0xFF);
}

/*****************************************************************************/
/* Working variables shared with ISR                                         */
/*                                                                           */
/* Options to use a message based ring buffer, but testing currently shows   */
/* that a single send/receive buffer with ISR driven tx & rx give good       */
/* performance.                                                              */
/*                                                                           */
/* To be a good neighbour, buffer limited to 64bytes.  The message format    */
/* allows upto around 270, but 64 is enough for the largest observed 12-zone */
/* EvoTouch system, including schedule updates (which always appear to be    */
/* broken into chunks).                                                      */
/*                                                                           */
/*****************************************************************************/
#define MAX_EVOHOME_MSG 64
static volatile uint8_t msg[MAX_EVOHOME_MSG]={0};
static volatile uint8_t msg_pos = 0;
static volatile uint8_t msg_len = 0;
static volatile uint8_t chksum=0;
static volatile uint8_t abort=0;
static uint8_t debug=0;

static volatile uint8_t work_index;
static volatile uint8_t work[2];

/*****************************************************************************/
/* Primary states                                                            */
/*****************************************************************************/
#define STATE_OFF    0  // Must be zero for easy active check
#define STATE_IDLE   1
#define STATE_LISTEN 2
#define STATE_RX     3
#define STATE_TX     4
#define STATE_CAL    5

static volatile uint8_t state=STATE_OFF;

/*****************************************************************************/
/* The Rx & Tx substates need to be in the order of progression              */
/*****************************************************************************/
#define SUBSTATE_IDLE         0
#define SUBSTATE_RX_PREAMBLE  1
#define SUBSTATE_RX_DATA      2
#define SUBSTATE_RX_POSTAMBLE 3
#define SUBSTATE_RX_DONE      4
#define SUBSTATE_TX_PREAMBLE  5
#define SUBSTATE_TX_DATA      6
#define SUBSTATE_TX_POSTAMBLE 7

static volatile uint8_t substate=SUBSTATE_IDLE;

/*****************************************************************************/
/* Macro to be used for all state manipulation                               */
/*****************************************************************************/
#define SET_STATE(X,Y) {substate=(Y);state=(X);if(debug){if (((state==STATE_RX)&&(substate > SUBSTATE_RX_PREAMBLE))||(state==STATE_TX)){LED_ON();}else{LED_OFF();}}};
#define SET_SUBSTATE(Z) {SET_STATE(state,(Z))}


/*****************************************************************************/
/* Rx abort handling                                                         */
/*                                                                           */
/* Either aborts are silent, or we generate diagnostics that may be useful   */
/* in diagnosing reception and code problems.                                */
/*****************************************************************************/
#define RX_ABORT(X) {if (debug) {abort=(X);SET_SUBSTATE(SUBSTATE_RX_DONE);}else{SET_STATE(STATE_LISTEN,SUBSTATE_IDLE);}}

/*****************************************************************************/
/* Fixed message segments                                                    */
/*****************************************************************************/
#define PREAMBLE_LEN 10
uint8_t preamble[PREAMBLE_LEN] = {0x55,0x55,0x55,0x55,0x55,0xFF,0x00,0x33,0x55,0x53};
#define MATCH_OFFSET 6

#define POSTAMBLE_LEN 4
uint8_t postamble[POSTAMBLE_LEN] = {0x35,0x55,0x55,0x55};

/*****************************************************************************/
/* Honeywell protocol uses 38K4 8N1 async                                    */
/*****************************************************************************/
#define BAUD 38400
#define USE_2X
#include <util/setbaud.h>
void uart1_initialize(void);

/*****************************************************************************/
/* Configure chip for true async opertion to USART                           */
/*****************************************************************************/
const uint8_t PROGMEM EVOHOME_CFG_ASYNC_38400[] = {
  CC1100_IOCFG2,    0x0D, // 00 GDO2 pin config:   0D: Async Rx data output
  CC1100_IOCFG0,    0x2D, // 02 GDO0 pin config:   2E: Tri-state (used as input during Tx)   (other code suggests 0x2D

  CC1100_FIFOTHR,   0x01, // 03 FIFO Threshhold                   Not used
  CC1100_PKTLEN,    0xff, // 06 Packet length
  CC1100_PKTCTRL1,  0x00, // 07 Packet automation control         00:no crc/addr
  CC1100_PKTCTRL0,  0x32, // 08 Packet automation control         async + infinite
  CC1100_FSCTRL1,   0x06, // 0B Frequency synthesizer control

  CC1100_FREQ2,     0x21, // 0D Frequency control word, high byte 868.4MHz
  CC1100_FREQ1,     0x65, // 0E Frequency control word, middle byte
  CC1100_FREQ0,     0x6c, // 0F Frequency control word, low byte
  CC1100_MDMCFG4,   0x6a, // 10 Modem configuration
  CC1100_MDMCFG3,   0x83, // 11 Modem configuration
  CC1100_MDMCFG2,   0x10, // 12 Modem configuration      GFSK/No Manchester, preamble or sync
  CC1100_MDMCFG1,   0x22, // 13 Modem configuration
  CC1100_DEVIATN,   0x50, // 15 Modem deviation setting

  CC1100_MCSM0,     0x18, // 18 Main Radio Cntrl State Machine config
  CC1100_FOCCFG,    0x16, // 19 Frequency Offset Compensation config
  CC1100_AGCCTRL2,  0x43, // 1B AGC control
  CC1100_AGCCTRL1,  0x40, // 1C AGC control
  CC1100_AGCCTRL0,  0x91, // 1D AGC control
  CC1100_FSCAL3,    0xe9, // 23 Frequency synthesizer calibration
  CC1100_FSCAL2,    0x2a, // 24 Frequency synthesizer calibration
  CC1100_FSCAL1,    0x00, // 25 Frequency synthesizer calibration
  CC1100_FSCAL0,    0x1f, // 26 Frequency synthesizer calibration
  CC1100_PATABLE,   0x50  // 3E
};

/*****************************************************************************/
/* Major C1100 state change routines                                         */
/*****************************************************************************/

/*****************************************************************************/
/* In listen mode we're looking for valid messages                           */
/*****************************************************************************/
void evohome_LISTEN(void)
{
  SET_STATE(STATE_LISTEN,SUBSTATE_IDLE);
  UCSR1B = _BV(RXCIE1) | _BV(RXEN1); // RX Complete IRQ enabled, Receiver enable
  ccRX();
  EIMSK &= ~_BV(CC1100_INT);                 // disable INT - we'll poll...
}

/*****************************************************************************/
/* In transmit we're going to send out a message                             */
/*****************************************************************************/
void evohome_TX(void)
{
  SET_STATE(STATE_TX,SUBSTATE_IDLE);
  ccTX();
  EIMSK &= ~_BV(CC1100_INT);                 // disable INT - we'll poll...

  /***************************************************************************/
  /* Note, the moment we enable the Tx empty interrupt the ISR will be       */
  /* called to load the first byte.  We must therefore be sure we are ready, */
  /* including and state transitions made before calling evohome_TX()        */
  /***************************************************************************/
  UCSR1B = _BV(UDRIE1)|_BV(TXEN1); // TX Empty IRQ enabled, Transmitter enable
}

/*****************************************************************************/
/* In idle, the chip is configured, but we're not doing anything             */
/*****************************************************************************/
void evohome_IDLE(void)
{
  SET_STATE(STATE_IDLE,SUBSTATE_IDLE);
  UCSR1B = 0;                      // Disable Tx & Rx
  ccStrobe( CC1100_SIDLE );
  EIMSK &= ~_BV(CC1100_INT);                 // disable INT - we'll poll...
}

/*****************************************************************************/
/* CC1100 init, where we reset and load our CC110x config                    */
/*****************************************************************************/
void rf_evohome_init(void)
{
  SET_BIT( CC1100_CS_DDR, CC1100_CS_PIN );

  CC1100_DEASSERT;                           // Toggle chip select signal
  my_delay_us(30);
  CC1100_ASSERT;
  my_delay_us(30);
  CC1100_DEASSERT;
  my_delay_us(45);

  ccStrobe( CC1100_SRES );
  my_delay_us(100);

  for (uint8_t i = 0; i < sizeof(EVOHOME_CFG_ASYNC_38400); i += 2)
    cc1100_writeReg( pgm_read_byte(&EVOHOME_CFG_ASYNC_38400[i]),
                     pgm_read_byte(&EVOHOME_CFG_ASYNC_38400[i+1]) );


  ccStrobe( CC1100_SCAL );

  my_delay_ms(4);

  uart1_initialize();

  SET_STATE(STATE_IDLE,SUBSTATE_IDLE);
}

/*****************************************************************************/
/* Main task.  We do little here, except for unloading receive messages      */
/*****************************************************************************/
void
rf_evohome_task(void)
{
  uint8_t m,n;

  switch (state)
  {
    case STATE_RX:
      {
        if (substate == SUBSTATE_RX_DONE)
        {
          /*******************************************************************/
          /* Depending on settings, generate error info                      */
          /*******************************************************************/
          if (debug && abort)
          {
            DS("v!");
            DC(abort);
            DC(':');
          }
          else
          {
            DS( "vr" );
          }

          for (n=0;n<msg_pos;n++)
          {
            m = (msg[n]>>4)&0xf;
            DC((m < 10 ? '0'+m : 'A'+m-10));
            m = (msg[n] & 0xf);
            DC((m < 10 ? '0'+m : 'A'+m-10));
          }
          DNL();

  #if 0
          /*******************************************************************/
          /* Consider adding a calibrate sequence before continuting.        */
          /* However, for Evohome, you can often get two large messages sent */
          /* one after another from a single source, so we have to be ready  */
          /* for the next messsage ASAP.  Doing calibration here can cause   */
          /* message loss.                                                   */
          /*                                                                 */
          /* Better to do periodically                                       */
          /*******************************************************************/
          ccStrobe( CC1100_SCAL );
          my_delay_ms(1);
  #endif
          evohome_LISTEN();
        }
      } break;

    default:
      {
      } break;
  }
};

/*****************************************************************************/
/* In the func routine we process requests from the user                     */
/*****************************************************************************/
void rf_evohome_func(char *in)
{
  switch (in[1])
  {
    case 'd':
    case 'l':
      {
        if (in[1]=='d')
        {
          debug=1;
        }
        else
        {
          debug=0;
        }

        if (state==STATE_OFF)
        {
          rf_evohome_init();
        }

        if (state==STATE_IDLE)
        {
          evohome_LISTEN();
        }

        DS("va"); // Ack
        DNL();

      } break;

    case 's':
      {
        if (state==STATE_OFF)
        {
          rf_evohome_init();
        }

        /*********************************************************************/
        /* Decide if channel is free.  If not indicate busy, and allow app   */
        /* to retry (if needed we could accept and schedule, but it will     */
        /* require extra buffer space)                                       */
        /*********************************************************************/
        if ((state==STATE_IDLE) || (state==STATE_LISTEN))
        {
          if (state==STATE_LISTEN)
          {
            evohome_IDLE();
          }

          msg_len=fromhex(&in[2],(uint8_t *)msg,sizeof(msg));

          /*******************************************************************/
          /* Enable Tx, which will also lead to ISR being called to load the */
          /* first byte                                                      */
          /*                                                                 */
          /* Ack now because we have committed to transmit                   */
          /*******************************************************************/
          evohome_TX();
          DS("va"); // Ack
          DNL();
        }
        else
        {
          DS("vb"); // Busy
          DNL();
        }
      } break;

    case 0x00:
      {
        /***********************************************************************/
        /* Off                                                                 */
        /***********************************************************************/
        debug=0;
        evohome_IDLE();
        SET_STATE(STATE_OFF,SUBSTATE_IDLE);
        DS("va"); // Ack
        DNL();
      } break;

    default:
      {
        /***********************************************************************/
        /* Unknown command                                                     */
        /***********************************************************************/
        DS("v?"); // NAck
        DNL();

      } break;
  }
}

/*****************************************************************************/
/* Program the UART, but don't yet enable Tx or Rx                           */
/*****************************************************************************/
void uart1_initialize(void) {
  // initialize I/O pins
  DDRD |= _BV( 3 ); // PD3: TX out

  // initialize the USART
  UBRR1H = UBRRH_VALUE;
  UBRR1L = UBRRL_VALUE;
  UCSR1A = 0;
#if USE_2X
  UCSR1A |= _BV(U2X1);
#endif

  UCSR1B = 0; // Tx & Rx disabled
  UCSR1C = _BV(UCSZ10) | _BV(UCSZ11); // 8 Bit, n, 1 Stop
}

/*****************************************************************************/
/* Transmit interrupt function                                               */
/*                                                                           */
/* All the transmit behaviour is driven of the data register empty interrupt.*/
/* This ensures we get nice tight outbound messages.                         */
/*****************************************************************************/
ISR(USART1_UDRE_vect)
{
  uint8_t data;

  /***************************************************************************/
  /* Failsafe logic to protect against stray ISR                             */
  /***************************************************************************/
  if (state != STATE_TX)
  {
    evohome_LISTEN();
  }
  else
  {
    /*************************************************************************/
    /* Get down to the business of sending data                              */
    /*************************************************************************/
    switch (substate)
    {
      case SUBSTATE_IDLE:
      {
        /*********************************************************************/
        /* This is the startup case                                          */
        /*********************************************************************/
        msg_pos = 0;
        SET_SUBSTATE(SUBSTATE_TX_PREAMBLE);

        /*********************************************************************/
        /* Deliberate run through into TX to send first byte                 */
        /*********************************************************************/
      }

      case SUBSTATE_TX_PREAMBLE:
      {
        /*********************************************************************/
        /* Send permable bytes                                               */
        /*********************************************************************/
        UDR1=preamble[msg_pos++];

        if (msg_pos >= PREAMBLE_LEN)
        {
          SET_SUBSTATE(SUBSTATE_TX_DATA);
          chksum=0;
          work_index=0;
          msg_pos=0;
        }
      } break;

      case SUBSTATE_TX_DATA:
      {
        /*********************************************************************/
        /* Encode each byte in turn                                          */
        /*********************************************************************/
        if (work_index==0)
        {
          /*******************************************************************/
          /* Encode either a byte of data, or the checksum                   */
          /*******************************************************************/
          if (msg_pos < msg_len)
          {
            chksum+=msg[msg_pos];
            data=msg[msg_pos];
          }
          else
          {
            data=-chksum;
          }

          work[0]=MANCHESTER_ENCODE(((data & 0xF0)>>4));
          work[1]=MANCHESTER_ENCODE((data & 0xF));
        }

        /*********************************************************************/
        /* Send byte                                                         */
        /*********************************************************************/
        UDR1=work[work_index++];

        /*********************************************************************/
        /* Next message byte                                                 */
        /*********************************************************************/
        if (work_index==2)
        {
          msg_pos++;
          work_index=0;

          /*******************************************************************/
          /* If we're at the end move to postamble                           */
          /*******************************************************************/
          if (msg_pos > msg_len)
          {
            SET_SUBSTATE(SUBSTATE_TX_POSTAMBLE);
            msg_pos=0;
          }
        }
      } break;

      case SUBSTATE_TX_POSTAMBLE:
      {
        /*********************************************************************/
        /* Only test for postamble done _AFTER_ last byte has gone           */
        /*********************************************************************/
        if (msg_pos >= POSTAMBLE_LEN)
        {
          /*******************************************************************/
          /* Once we're done, move back to listen state, which will disable  */
          /* the Tx empty int.  (We used to move to TX_DONE so task routine  */
          /* could generate ack, but we now do this when we commit to        */
          /* transmit.)                                                      */
          /*******************************************************************/
          evohome_LISTEN();
          msg_pos=0;
        }
        else
        {
          /*******************************************************************/
          /* Send postamble bytes                                            */
          /*******************************************************************/
          UDR1=postamble[msg_pos++];
        }

      } break;

      default:
      {
        /*********************************************************************/
        /* We should never get here.  If we do, drop back to listen          */
        /*********************************************************************/
        evohome_LISTEN();
      } break;
    }
  }

}

/*****************************************************************************/
/* The bulk of the receive work is done in the Rx ISR, but using the USART   */
/* actually means there is very little to do beyond looking for message      */
/* boundaries and decoding                                                   */
/*****************************************************************************/
ISR(USART1_RX_vect) {

  uint8_t errors = UCSR1A;
  uint8_t data   = UDR1;
  uint8_t nib1,nib2;


  /***************************************************************************/
  /* We are willing to consider data if we are looking for preamble or if    */
  /* we're working on a message                                              */
  /***************************************************************************/
  if ((state == STATE_LISTEN) ||
      ((state == STATE_RX) && (substate != SUBSTATE_RX_DONE)))
  {
    /***************************************************************************/
    /* Error cause an abort                                                    */
    /***************************************************************************/
    if (errors & (_BV(FE1)|_BV(DOR1)))
    {
      if (state == STATE_RX)
      {
        if (substate > SUBSTATE_RX_PREAMBLE)
        {
          if (errors & _BV(FE1))
          {
            /*****************************************************************/
            /* Abort indicating framing error                                */
            /*****************************************************************/
            RX_ABORT('F');
          }
          else
          {
            /*****************************************************************/
            /* Abort indicating overrun                                      */
            /*****************************************************************/
            RX_ABORT('O');
          }
        }
        else
        {
          SET_STATE(STATE_LISTEN,SUBSTATE_IDLE);
        }
      }
    }
    else
    {
      switch (substate)
      {
        case SUBSTATE_IDLE:
        {
          /*********************************************************************/
          /* In this state we are looking for first pre-amble byte             */
          /*********************************************************************/
          msg_pos = MATCH_OFFSET;
          if (data==preamble[msg_pos])
          {
            SET_STATE(STATE_RX, SUBSTATE_RX_PREAMBLE);
            abort=0;
            msg_pos++;
          }
        } break;

        case SUBSTATE_RX_PREAMBLE:
        {
          /*********************************************************************/
          /* We're looking to either complete preamble or abandon              */
          /*********************************************************************/
          if (msg_pos < PREAMBLE_LEN)
          {
            if (data==preamble[msg_pos])
            {
              msg_pos++;
            }
            else
            {
              SET_STATE(STATE_LISTEN,SUBSTATE_IDLE);
            }
          }

          /*********************************************************************/
          /* If preamble is complete, then move to Rx                          */
          /*********************************************************************/
          if (msg_pos == PREAMBLE_LEN)
          {
            SET_SUBSTATE(SUBSTATE_RX_DATA);
            msg_pos=0;
            work_index=0;
            chksum=0;
          }
        } break;

        case SUBSTATE_RX_DATA:
        {
          /**********************************************************************/
          /* Abort if message is over length                                    */
          /**********************************************************************/
          if (msg_pos >= MAX_EVOHOME_MSG)
          {
            /*****************************************************************/
            /* Abort indicating long message                                 */
            /*****************************************************************/
            RX_ABORT('L');
          }
          else
          {
            /*********************************************************************/
            /* Accumulate 2 work bytes                                           */
            /*********************************************************************/
            work[work_index++]=data;

            /*******************************************************************/
            /* Check for normal end                                            */
            /*******************************************************************/
            if (data == postamble[0])
            {
              /*****************************************************************/
              /* Check we've got at least one data byte, then check checksum   */
              /*****************************************************************/
              if ((msg_pos < 2) || chksum)
              {
                /*************************************************************/
                /* Abort indicating checksum problem                         */
                /*************************************************************/
                RX_ABORT('C');
              }
              else
              {
                /*************************************************************/
                /* Remove checksum byte and mark as done                     */
                /*************************************************************/
                msg_pos--;
                SET_SUBSTATE(SUBSTATE_RX_DONE);
              }
            }
            else if (work_index==2)
            {
              work_index=0;

              /**********************************************************************/
              /* Decode next character                                              */
              /**********************************************************************/
              nib1=MANCHESTER_DECODE(work[0]);
              nib2=MANCHESTER_DECODE(work[1]);

              /**********************************************************************/
              /* Check for violations                                               */
              /**********************************************************************/
              if ((nib1==0xFF) || (nib2==0xFF))
              {
                /***************************************************************/
                /* Abort indicating manchester error                           */
                /***************************************************************/
                RX_ABORT('M');
              }
              else
              {
                /********************************************************************/
                /* Form message byte & save                                         */
                /********************************************************************/
                msg[msg_pos]=((nib1<<4) | nib2);
                chksum+=msg[msg_pos];
                msg_pos++;
              }
            }
          }
        } break;

        case SUBSTATE_RX_DONE:
        default:
        {
          /*********************************************************************/
          /* Wait for message to completed message to be unloaded              */
          /*                                                                   */
          /* Any received data will either be noise or early preamble which we */
          /* can afford to miss                                                */
          /*********************************************************************/
        } break;
      }
    }
  }
}

#endif
