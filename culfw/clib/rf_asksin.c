#include "board.h"
#ifdef HAS_ASKSIN
#include <string.h>
#include <avr/pgmspace.h>
#include "cc1100.h"
#include "delay.h"
#include "rf_receive.h"
#include "display.h"

#include "rf_asksin.h"

const uint8_t PROGMEM ASKSIN_CFG[50] = {
     0x00, 0x07,
     0x02, 0x06,
     0x03, 0x0D,
     0x04, 0xE9,
     0x05, 0xCA,
     0x07, 0x0C,
     0x0B, 0x06,
     0x0D, 0x21,
     0x0E, 0x65,
     0x0F, 0x6A,
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

void rf_asksin_init(void) {

  EIMSK &= ~_BV(CC1100_INT);                 // disable INT - we'll poll...
  SET_BIT( CC1100_CS_DDR, CC1100_CS_PIN );   // CS as output

  CC1100_DEASSERT;                           // Toggle chip select signal
  my_delay_us(30);
  CC1100_ASSERT;
  my_delay_us(30);
  CC1100_DEASSERT;
  my_delay_us(45);

  ccStrobe( CC1100_SRES );                   // Send SRES command
  my_delay_us(100);

  // load configuration
  for (uint8_t i = 0; i<50; i += 2) {
       
       if (pgm_read_byte( &ASKSIN_CFG[i] )>0x40)
	    break;

       cc1100_writeReg( pgm_read_byte(&ASKSIN_CFG[i]), pgm_read_byte(&ASKSIN_CFG[i+1]) );
  }
  
  ccStrobe( CC1100_SCAL );

  my_delay_ms(1);
}

void rf_asksin_task(void) {
     uint8_t enc[MAX_ASKSIN_MSG];
     uint8_t dec[MAX_ASKSIN_MSG];
     uint8_t rssi;
     uint8_t l;

     if ((tx_report & 0x100) == 0)
	  return;

     // see if a CRC OK pkt has been arrived
     if (bit_is_set( CC1100_IN_PORT, CC1100_IN_PIN )) {

	  enc[0] = cc1100_readReg( 0xbf ); // read len

	  if (enc[0]>=MAX_ASKSIN_MSG)
	       enc[0] = MAX_ASKSIN_MSG-1;
	  
	  CC1100_ASSERT;
	  cc1100_sendbyte( 0xff );
	  
	  for (uint8_t i=0; i<enc[0]; i++) {
	       enc[i+1] = cc1100_sendbyte( 0 );
	  }
	  
	  rssi = cc1100_sendbyte( 0 );

	  CC1100_DEASSERT;

/*
	  if (rssi>=128)
	       rssi -= 256;
	  
	  rssi /= 2;
	  rssi -= 74;
*/
	  
	  dec[0] = enc[0];
	  dec[1] = (~enc[1]) ^ 0x89;

	  for (l = 2; l < dec[0]; l++)
	       dec[l] = (enc[l-1] + 0xdc) ^ enc[l];

	  dec[l] = enc[l] ^ dec[2];


	  DC('A');
     
	  for (uint8_t i=0; i<=dec[0]; i++)
	       DH2( dec[i] );

	  if (tx_report & REP_RSSI)
	       DH2(rssi);
	  
	  DNL();

	  ccStrobe( CC1100_SFRX  );
     }
     
     switch (cc1100_readReg( CC1100_MARCSTATE )) {
	       
	  // IDLE!
     case 17:
     case 1:
	  ccStrobe( CC1100_SFRX  );
	  ccStrobe( CC1100_SIDLE );
	  ccStrobe( CC1100_SNOP  );
	  ccStrobe( CC1100_SRX   );
	  break;
	  
     }
     

}

void asksin_send(char *in) {
     uint8_t enc[MAX_ASKSIN_MSG];
     uint8_t dec[MAX_ASKSIN_MSG];
     uint8_t l;

     uint8_t hblen = fromhex(in+1, dec, MAX_ASKSIN_MSG-1);

     if ((hblen-1) != dec[0]) {
//	  DS_P(PSTR("LENERR\r\n"));
	  return;
     }

     // in AskSin mode already?
     if ((tx_report & 0x100) == 0) {
	  rf_asksin_init();
	  my_delay_ms(3);             // 3ms: Found by trial and error
     }

     ccStrobe(CC1100_SIDLE);
     ccStrobe(CC1100_SFRX );
     ccStrobe(CC1100_SFTX );

     // "crypt"

     enc[0] = dec[0];
     enc[1] = (~dec[1]) ^ 0x89;

     for (l = 2; l < dec[0]; l++)
	  enc[l] = (enc[l-1] + 0xdc) ^ dec[l];
     
     enc[l] = dec[l] ^ dec[2];

     // send
     CC1100_ASSERT;
     cc1100_sendbyte(CC1100_WRITE_BURST | CC1100_TXFIFO);

     for(uint8_t i = 0; i < hblen; i++) {
	  cc1100_sendbyte(enc[i]);
     }

     CC1100_DEASSERT;

     ccStrobe( CC1100_SFRX  );
     ccStrobe( CC1100_STX   );
     
     while( cc1100_readReg( CC1100_MARCSTATE ) != 1 )
	  my_delay_ms(5);
     
     set_txrestore();
}


#endif
