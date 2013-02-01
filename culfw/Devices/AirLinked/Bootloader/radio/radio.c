#include <stdio.h>
#include <string.h>

#include "radio.h"
#include "cc1100.h"

#include "../chipdef.h"
#include "../../board.h"

#include <util/delay.h>

static uint8_t seq = 0;
static uint8_t rseq = 0xff;

typedef struct {
  uint8_t putoff;
  uint8_t getoff;
  uint8_t nbytes;       // Number of data bytes
  char buf[TTY_BUFSIZE];
} rb_t;

rb_t Tx, Rx;

const uint8_t CC1100_CFG[50] = {
     0x00, 0x07,
     0x02, 0x2e,
     0x03, 0x0d,
     0x04, 0x05,
     0x05, 0x03,
     0x07, 0x0C,
     0x0B, 0x06,
     0x0D, 0x21,
     0x0E, 0x36,
     0x0F, 0x27,
     0x10, 0xC8,
     0x11, 0x93,
     0x12, 0x03,
     0x15, 0x34,
     0x17, 0x30, // always go into IDLE
     0x18, 0x18,
     0x19, 0x16,
     0x1B, 0x43,
     0x21, 0x56,
     0x25, 0x00,
     0x26, 0x11,
     0x2D, 0x35,
     0x3e, 0xc3,
     0xff
};


void rb_reset(rb_t *rb) {
  rb->getoff = rb->putoff = rb->nbytes = 0;
}

void rb_put(rb_t *rb, uint8_t data) {
  if(rb->nbytes >= TTY_BUFSIZE) {
    return;
  }
  rb->nbytes++;
  rb->buf[rb->putoff++] = data;
  if(rb->putoff == TTY_BUFSIZE)
    rb->putoff = 0;
}

uint8_t rb_get(rb_t *rb) {
  uint8_t ret;
  if(rb->nbytes == 0)
    return 0;
  rb->nbytes--;
  ret = rb->buf[rb->getoff++];
  if(rb->getoff == TTY_BUFSIZE)
    rb->getoff = 0;
  return ret;
}

void spi_init(void) {

#ifdef PRR0
  PRR0 &= ~_BV(PRSPI);
#endif
  SPI_PORT |= _BV(SPI_SCLK);
  SPI_DDR  |= (_BV(SPI_MOSI) | _BV(SPI_SCLK) | _BV(SPI_SS));
  SPI_DDR  &= ~_BV(SPI_MISO);

  SPCR  = _BV(MSTR) | _BV(SPE);
//  SPSR |= _BV(SPI2X);

}

uint8_t cc1100_sendbyte(uint8_t data) {
  SPDR = data;                  // send byte
  while (!(SPSR & _BV (SPIF))); // wait until transfer finished
  return SPDR;
}

//--------------------------------------------------------------------
uint8_t cc1100_readReg(uint8_t addr) {
  CC1100_ASSERT;
  cc1100_sendbyte( addr|CC1100_READ_BURST );
  uint8_t ret = cc1100_sendbyte( 0 );
  CC1100_DEASSERT;
  return ret;
}

void cc1100_writeReg(uint8_t addr, uint8_t data) {
  CC1100_ASSERT;
  cc1100_sendbyte( addr|CC1100_WRITE_BURST );
  cc1100_sendbyte( data );
  CC1100_DEASSERT;
}


//--------------------------------------------------------------------
uint8_t ccStrobe(uint8_t strobe) {
  CC1100_ASSERT;
  uint8_t ret = cc1100_sendbyte( strobe );
  CC1100_DEASSERT;
  return ret;
}


void cc_init(uint16_t syncword) {

  EIMSK &= ~_BV(CC1100_INT);                 // disable INT - we'll poll...
  SET_BIT( CC1100_CS_DDR, CC1100_CS_PIN );   // CS as output

  CC1100_DEASSERT;                           // Toggle chip select signal
  _delay_us(30);
  CC1100_ASSERT;
  _delay_us(30);
  CC1100_DEASSERT;
  _delay_us(45);

  ccStrobe( CC1100_SRES );                   // Send SRES command
  _delay_us(200);

  // load configuration
  for (uint8_t i = 0; i<50; i += 2) {

    if (CC1100_CFG[i]>0x40)
      break;

    cc1100_writeReg( CC1100_CFG[i], CC1100_CFG[i+1] );
  }

  ccStrobe( CC1100_SCAL );

  while( cc1100_readReg( CC1100_MARCSTATE ) != 1 )
    _delay_ms(5);

  return;

  uint8_t cnt = 0xff;
  while(cnt-- && (ccStrobe( CC1100_SRX ) & 0x70) != 1)
    _delay_us(10);

}

void radio_init(void) {

  spi_init();

  cc_init( 0 );

  rb_reset( &Tx );
  rb_reset( &Rx );
}

void DC (uint8_t byte) {
//  while (!(UART_STATUS & (1<<UART_TXREADY)));
//  UART_DATA = byte;
}

void radio_flush( void ) {
  uint8_t bytes = Tx.nbytes;
  uint8_t buf[60];

  if (!bytes)
    return;

  if (bytes>60)
    bytes = 60;

//  DC( 'f' );

  uint8_t SEQ = seq++ & 0x7f;

  for(uint8_t i = 0; i < bytes; i++) {
    buf[i] = rb_get(&Tx);
  }

  DC( '.' );

  uint8_t csend = 0;

resend:

  // FATAL, leave CC in IDLE ...
  if (csend++>3)
    return;

  ccStrobe(CC1100_SIDLE);
  ccStrobe(CC1100_SFTX);

  // fill FIFO
  CC1100_ASSERT;
  cc1100_sendbyte(CC1100_WRITE_BURST | CC1100_TXFIFO);

  cc1100_sendbyte(bytes+1);
  cc1100_sendbyte(SEQ);

  for(uint8_t i = 0; i < bytes; i++) {
    cc1100_sendbyte(buf[i]);
  }

  CC1100_DEASSERT;

  // send
  _delay_ms(5);
  ccStrobe(CC1100_STX);

  while( cc1100_readReg( CC1100_MARCSTATE ) != 1 )
    _delay_ms(5);

rerecv:
  ccStrobe( CC1100_SFRX  );
  ccStrobe( CC1100_SIDLE );
  ccStrobe( CC1100_SRX   );
//  while (cc1100_readReg( CC1100_MARCSTATE ) != 13);

  uint8_t to = 0;
  // wait for ACK packet
  while(!bit_is_set( CC1100_IN_PORT, CC1100_IN_PIN )) {
//  while((cc1100_readReg(CC1100_PKTSTATUS) & CC1100_LQI_CRC_OK_BM) == 0) {
    if (to++>25) {
      DC( 'T' );
      goto resend;
    }
    _delay_ms(10);
  }

  // verify ACK len 
  uint8_t len  = (cc1100_readReg( CC1100_RXFIFO ) & 0x7f);
  uint8_t rSEQ = len ? cc1100_readReg( CC1100_RXFIFO ) : 0;
/*
  if (len == 1) {
    if ((SEQ | 0x80) != rSEQ) {
      DC( 'X' );
      goto rerecv;
    }
  } else {
    DC( 'L' );
    DC( 'a'+len );
    if (((rseq+1)&0x7f) == rSEQ) {
      DC( '+' );
    } else {
      goto rerecv;
    }
  }
*/

  ccStrobe( CC1100_SFRX  );
  ccStrobe( CC1100_SIDLE );
  ccStrobe( CC1100_SRX   );
//  while (cc1100_readReg( CC1100_MARCSTATE ) != 13);

  DC( 13 );
  DC( 10 );
}

void radio_sendchar(uint8_t data) {
  rb_put( &Tx, data );

  if (Tx.nbytes>=60)
    radio_flush();
}

uint8_t radio_recvchar(void) {
  uint8_t bytes = 0;
  uint16_t loop = 0;

  radio_flush();

//  DC( 'r' );

  while (!Rx.nbytes) {

    // LED 
    if (loop++==0)
      LED_PORT ^= _BV( LED_PIN );

    // see if a CRC OK pkt has been arrived
    if(bit_is_set( CC1100_IN_PORT, CC1100_IN_PIN )) {
//    if(cc1100_readReg(CC1100_PKTSTATUS) & CC1100_LQI_CRC_OK_BM) {
      bytes = cc1100_readReg( CC1100_RXFIFO ) & 0x7f; // read len
      bytes--;

      uint8_t seq = cc1100_readReg( CC1100_RXFIFO );
 
      // recv an ACK?
      if (seq & 0x80) {
        DC( 'X' );
	goto leave;
      }

//      if (seq == ((rseq+1)&0x7f)) {
      if (seq != rseq) {

        rseq = seq;
        DC( 'R' ); DC( 'a'+bytes );
  
        CC1100_ASSERT;
        cc1100_sendbyte( CC1100_READ_BURST | CC1100_RXFIFO );
  
        for (uint8_t i=0; i<bytes; i++) {
             rb_put( &Rx, cc1100_sendbyte( 0 ));
        }
  
        CC1100_DEASSERT;
      } else {
        DC( 'Q' );
      }
  
      ccStrobe( CC1100_SFRX  );

      // ACK the received packet - only if we had payload
      if (bytes>0) {

        ccStrobe( CC1100_SIDLE );
        ccStrobe( CC1100_SFTX );
        // fill FIFO
        CC1100_ASSERT;
        cc1100_sendbyte(CC1100_WRITE_BURST | CC1100_TXFIFO);

        cc1100_sendbyte(1);
        cc1100_sendbyte(seq | 0x80);

        CC1100_DEASSERT;

        // send
        _delay_ms(5);
        ccStrobe(CC1100_STX);

        while( cc1100_readReg( CC1100_MARCSTATE ) != 1 )
         _delay_ms(5);

      } else {
        DC( 'B' );
      }

leave:
      ccStrobe( CC1100_SFRX  );
      ccStrobe( CC1100_SRX   );
//      while (cc1100_readReg( CC1100_MARCSTATE ) != 13);
      continue;
    }

    switch (cc1100_readReg( CC1100_MARCSTATE )) {
      case 13: 
        break;
      case 1:
      case 17:
      case 22:
        ccStrobe( CC1100_SFTX  );
        ccStrobe( CC1100_SFRX  );
        ccStrobe( CC1100_SIDLE );
        ccStrobe( CC1100_SRX   );
//        while (cc1100_readReg( CC1100_MARCSTATE ) != 13);
      default:
//        DC( ':' ); DC( 'd'+cc1100_readReg( CC1100_MARCSTATE ) );
        break;
    }

  }

  bytes = rb_get( &Rx );

  return bytes;
}

// check if other side a available
uint8_t radio_avail(void) {

  ccStrobe(CC1100_SIDLE);
  ccStrobe(CC1100_SFTX);

  // fill FIFO
  CC1100_ASSERT;
  cc1100_sendbyte(CC1100_WRITE_BURST | CC1100_TXFIFO);

  cc1100_sendbyte(6);
  cc1100_sendbyte(0x88); // magic
  cc1100_sendbyte('H');
  cc1100_sendbyte('E');
  cc1100_sendbyte('L');
  cc1100_sendbyte('L');
  cc1100_sendbyte('O');

  CC1100_DEASSERT;

  // send
  _delay_ms(5);
  ccStrobe(CC1100_STX);

  while( cc1100_readReg( CC1100_MARCSTATE ) != 1 )
    _delay_ms(5);

  ccStrobe( CC1100_SFRX  );
  ccStrobe( CC1100_SIDLE );
  ccStrobe( CC1100_SRX   );

  uint8_t to = 0;
  // wait for ACK packet
  while(!bit_is_set( CC1100_IN_PORT, CC1100_IN_PIN )) {
    if (to++>10) {
      return 0; // No answer!
    }
    _delay_ms(100);
  }

  // verify ACK len
  uint8_t len  = (cc1100_readReg( CC1100_RXFIFO ) & 0x7f);
  uint8_t rSEQ = len ? cc1100_readReg( CC1100_RXFIFO ) : 0;

  if ((len == 1) && (rSEQ == 0xaa)) { 
    ccStrobe( CC1100_SFRX  );
    ccStrobe( CC1100_SIDLE );
    ccStrobe( CC1100_SRX   );

    return 1;
  }

  return 0;
}

