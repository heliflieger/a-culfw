/*	I2CCUL - ProMini(ATmega328) as I2CSlave
	 
	Diggen85 (Benny_Stark@live.de)
	19.01.2017	-	Add Communication as I2C Slave
	29.01.2017 	- Cleanup, remove unneeded defines, added Makefile defines
							- Added IR
							- Changed CC1100OUT to ProMini A0 - PC0
	19.02.2018	- Changed Protocol definition
*/

#ifndef _BOARD_H
#define _BOARD_H

#include <avr/io.h>
#include <stdint.h>

/* CC1101 SPI */
#define SPI_PORT		PORTB
#define SPI_DDR			DDRB
#define SPI_SS			2									//ProMini D10
#define SPI_MISO		4									//ProMini D12
#define SPI_MOSI		3									//ProMini D11
#define SPI_SCLK		5									//ProMini D13 / Builtin LED

#define CC1100_CS_DDR		SPI_DDR
#define CC1100_CS_PORT  SPI_PORT
#define CC1100_CS_PIN		SPI_SS

/* CC1101 GDO0 Tx / Temperature Sensor */
#define CC1100_OUT_DDR		DDRC
#define CC1100_OUT_PORT         PORTC
#define CC1100_OUT_PIN          PC0
#define CC1100_OUT_IN           PINC 	//ProMini A0
#define CCTEMP_MUX              CC1100_OUT_PIN

/* CC1101 GDO2 Rx Interrupt */ 
#define CC1100_IN_DDR		DDRD
#define CC1100_IN_PORT          PIND
#define CC1100_IN_PIN           PD2
#define CC1100_IN_IN            PIND	//ProMini D2

#define CC1100_INT		INT0
#define CC1100_INTVECT          INT0_vect
#define CC1100_ISC		ISC00
#define CC1100_EICR             EICRA

/* externe LED */ 
#define LED_DDR                 DDRB
#define LED_PORT                PORTB
#define LED_PIN                 1 		//ProMini D9


// I2C Slave uses the same tty_buffer as UART
#define HAS_I2CSLAVE
#define TTY_BUFSIZE             128

#if defined(I2CSLAVE_ADDR) && defined(I2CSLAVE_HWADDR)
	#error define I2CSLAVE_ADDR=[SWADDR] or I2CSLAVE_HWADDR
#endif
#if defined(I2CSLAVE_ADDR)
	extern const uint8_t i2cSlaveAddr;
#endif

//	I2CADDR	A2	A1 ProMini Pin
//	0x70 =	0		0
//	0x71 =	0		1	
//	0x72 =	1		0
//	0x73 =	1		1
#if defined(I2CSLAVE_HWADDR)		//I2CSLAVE_ADDR_BASE && (A2<<2)(A1<<1)
	#define I2CSLAVE_ADDR_DDR			DDRC
	#define I2CSLAVE_ADDR_PORT		PORTC
	#define	I2CSLAVE_ADDR_PIN			PINC
	#define	I2CSLAVE_ADDR_A0			1			//ProMini A1
	#define	I2CSLAVE_ADDR_A1			2			//ProMini A2
	#define I2CSLAVE_ADDR_BASE		0x70
	extern uint8_t i2cSlaveAddr;
#endif


//define Boardname
#define BOARD_ID_STR            "I2CCUL868"
#define BOARD_ID_STR433         "I2CCUL433"

/* define this device as a 433 MHz one */
/* this isn't done like a CUL by reading a port pin but instead a fixed value of 0 for mark433_pin is used */ 
/* so unneeded Protocols wont be included */
#define MULTI_FREQ_DEVICE
#define MARK433_PIN 			mark433_pin
#define MARK433_BIT             0
extern const uint8_t mark433_pin;


/* IR Functions */
//#define HAS_IRRX 
//#define HAS_IRTX
#define IRMP_PORT			PORTD
#define IRMP_DDR      DDRD
#define IRMP_PIN      PIND
#define IRMP_BIT      4 					// PortD4 = ProMini D4 TSOPXX38
//#define F_INTERRUPTS  15625   		// interrupts per second, min: 10000, max: 20000 // Timer0: 0.008s = 8MHz/256/2   == 15625Hz
#define IRSND_OCx     IRSND_OC2B  // use OC2B/PD3 = ProMini D3 IR-LED


//define Protocols and options
#define RCV_BUCKETS            2      //                 RAM: 25b * bucket
#define FHTBUF_SIZE          174    //     	           RAM: 174b

#define FULL_CC1100_PA                // PROGMEM:  108b
#define HAS_CC1101_RX_PLL_LOCK_CHECK_TASK_WAIT
#define HAS_CC1101_PLL_LOCK_CHECK_MSG
#define HAS_CC1101_PLL_LOCK_CHECK_MSG_SW

#define HAS_MEMFN											//Memory Functions

#define HAS_RAWSEND                   // PROGMEM:  198b     RAM:  4b
#define HAS_RFNATIVE

#if defined (I2CCUL433)
/* Intertechno Send */
#define HAS_IT
/* Intertechno Receive einschalten */
#define HAS_INTERTECHNO
#define HAS_REVOLT
#define HAS_TCM97001
#define HAS_HOMEEASY
#define HAS_BELFOX
#define HAS_MANCHESTER
#define HAS_TX3
#define HAS_SOMFY_RTS
#endif

#if defined (I2CCUL868)
#define HAS_FASTRF                    // PROGMEM:  468b  RAM:  1b
#define HAS_ASKSIN
#define HAS_ASKSIN_FUP
#define HAS_MORITZ
#define HAS_RWE
#define HAS_ESA
#define HAS_HMS
#define LACROSSE_HMS_EMU          	// if you like HMS emulation for LaCrosse temp devices
#define HAS_UNIROLL
//#define HAS_FHT_80b                 // PROGMEM: 1374b, RAM: 90b
//#define HAS_FHT_8v                  // PROGMEM:  586b  RAM: 23b
//#define HAS_FHT_TF
//#define HAS_HOERMANN
//#define HAS_HOERMANN_SEND
//#define HAS_KOPP_FC
//#define HAS_ZWAVE                   // PROGMEM:  882
/* HAS_MBUS requires about 1kB RAM, if you want to use it you
   should consider disabling other unneeded features
   to avoid stack overflows
*/
//#define HAS_MBUS
#endif

#endif
