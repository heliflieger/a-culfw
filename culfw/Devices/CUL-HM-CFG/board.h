#ifndef BOARD_H 
#define BOARD_H

#define _BV(a) (1<<(a))
#define bit_is_set(sfr, bit) ((sfr) & _BV(bit))


//#define LONG_PULSE

#define TTY_BUFSIZE          128      // RAM: TTY_BUFSIZE*4
#define ARM
#define HAS_USB
#define USB_IsConnected		(USBD_GetState() == USBD_STATE_CONFIGURED)
#define USB_DESCRIPTOR_SN	'1'

#define HAS_FHT_80b
#define HAS_FHT_8v
//#define HAS_RF_ROUTER
#define HAS_CC1101_RX_PLL_LOCK_CHECK_TASK_WAIT
#define HAS_CC1101_PLL_LOCK_CHECK_MSG
#define HAS_CC1101_PLL_LOCK_CHECK_MSG_SW

#define FHTBUF_SIZE          174
#define RCV_BUCKETS            4      //                 RAM: 25b * bucket
#define RFR_DEBUG
#define FULL_CC1100_PA
#define HAS_RAWSEND
#define HAS_FASTRF
#define HAS_ASKSIN
#define HAS_ASKSIN_FUP
#define HAS_MORITZ
#define HAS_RWE
#define HAS_ESA
#define HAS_HMS
#define HAS_TX3
#define HAS_INTERTECHNO
#define HAS_UNIROLL
#define HAS_HOERMANN
#define HAS_SOMFY_RTS
//#define HAS_MAICO

//#define HAS_MBUS
//#define HAS_MEMFN

#if defined(HM_CFG)

#elif defined(HM_CFG_BL)
#define HM_CFG
#elif defined(bootloader_HM_CFG)
#undef  TTY_BUFSIZE
#define TTY_BUFSIZE          512
#define HM_CFG
#else
#define HM_CFG
#endif

#define BOARD_NAME 			"CUL-HM-CFG"
#define BOARD_ID_STR        "CUL-HM-CFG"


#define SPI_SS				(1<<11)
#define SPI_MISO			(1<<12)
#define SPI_MOSI			(1<<13)
#define SPI_SCLK			(1<<14)

#define CC1100_CS_PIN		11
#define CC1100_CS_BASE		AT91C_BASE_PIOA
#define CC1100_OUT_PIN      20
#define CC1100_OUT_BASE     AT91C_BASE_PIOA
#define CC1100_IN_PIN       19
#define CC1100_IN_BASE		AT91C_BASE_PIOA
#define CC1100_IN_PORT      AT91C_BASE_PIOA->PIO_PDSR

#define BOOTLOADER_PIN		(1<<9)

//------------------------------------------------------------------------------
//         Headers
//------------------------------------------------------------------------------
#include "AT91SAM7S128.h"

//------------------------------------------------------------------------------
//         Definitions
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/// \page "SAM7X-EK - Operating frequencies"
/// This page lists several definition related to the board operating frequency
/// (when using the initialization done by board_lowlevel.c).
/// 
/// !Definitions
/// - BOARD_MAINOSC
/// - BOARD_MCK

/// Frequency of the board main oscillator.
#define BOARD_MAINOSC           18432000

/// Master clock frequency (when using board_lowlevel.c).
#define BOARD_MCK               48000000
//------------------------------------------------------------------------------

/// Indicates the chip has a UDP controller.
#define BOARD_USB_UDP

/// Indicates the D+ pull-up is externally controlled.
#define BOARD_USB_PULLUP_EXTERNAL
#define PIN_USB_PULLUP {1 << 16, AT91C_BASE_PIOA, AT91C_ID_PIOA, PIO_OUTPUT_0, PIO_DEFAULT}

/// Number of endpoints in the USB controller.
#define BOARD_USB_NUMENDPOINTS                  4

/// Returns the maximum packet size of the given endpoint.
/// \param i  Endpoint number.
/// \return Maximum packet size in bytes of endpoint.
#define BOARD_USB_ENDPOINTS_MAXPACKETSIZE(i)    ((i == 0) ? 8 : 64)

/// Returns the number of FIFO banks for the given endpoint.
/// \param i  Endpoint number.
/// \return Number of FIFO banks for the endpoint.
#define BOARD_USB_ENDPOINTS_BANKS(i)            (((i == 0) || (i == 3)) ? 1 : 2)

/// USB attributes configuration descriptor (bus or self powered, remote wakeup)
//#define BOARD_USB_BMATTRIBUTES                  USBConfigurationDescriptor_SELFPOWERED_NORWAKEUP
#define BOARD_USB_BMATTRIBUTES                  USBConfigurationDescriptor_BUSPOWERED_NORWAKEUP

//------------------------------------------------------------------------------

#define PINS_DBGU  { (1<<9)|(1<<10), AT91C_BASE_PIOA, AT91C_ID_PIOA, PIO_PERIPH_A, PIO_DEFAULT}

/// LED #0 pin definition.A8
#define PIN_LED_0  {1 << 24, AT91C_BASE_PIOA, AT91C_ID_PIOA, PIO_OUTPUT_1, PIO_DEFAULT}

#define PINS_LEDS  PIN_LED_0


/// USART0 RXD pin definition.
#define PIN_USART0_RXD  {1 << 0, AT91C_BASE_PIOA, AT91C_ID_PIOA, PIO_PERIPH_A, PIO_DEFAULT}
/// USART0 TXD pin definition.
#define PIN_USART0_TXD  {1 << 1, AT91C_BASE_PIOA, AT91C_ID_PIOA, PIO_PERIPH_A, PIO_DEFAULT}
/// USART0 SCK pin definition.


#define AT91C_BASE_SPI0		AT91C_BASE_SPI
#define AT91C_ID_SPI0		AT91C_ID_SPI

/// Indicates chip has an EFC.
#define BOARD_FLASH_EFC
/// Address of the IAP function in ROM.
#define BOARD_FLASH_IAP_ADDRESS         0x300008
//------------------------------------------------------------------------------

#endif //#ifndef BOARD_H

