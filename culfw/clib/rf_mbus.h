#ifndef _RF_MBUS_H
#define _RF_MBUS_H

#include "board.h"

#ifdef HAS_MBUS

#include "mbus/mbus_defs.h"

void rf_mbus_task(void);
void rf_mbus_func(char *in);

extern uint8_t mbus_mode;
#define	WMBUS_NONE 	0

#define RX_FIFO_THRESHOLD         0x07
#define RX_FIFO_START_THRESHOLD   0x00
#define RX_FIFO_SIZE              64
#define RX_OCCUPIED_FIFO          32    // Occupied space
#define RX_AVAILABLE_FIFO         32    // Free space

#define FIXED_PACKET_LENGTH       0x00
#define INFINITE_PACKET_LENGTH    0x02
#define INFINITE                  0
#define FIXED                     1
#define MAX_FIXED_LENGTH          256

#define RX_STATE_ERROR            3

typedef struct RXinfoDescr {
    uint8  lengthField;         // The L-field in the WMBUS packet
    uint16 length;              // Total number of bytes to to be read from the RX FIFO
    uint16 bytesLeft;           // Bytes left to to be read from the RX FIFO
    uint8 *pByteIndex;          // Pointer to current position in the byte array
    uint8 format;               // Infinite or fixed packet mode
    uint8 start;                // Start of Packet
    uint8 complete;             // Packet received complete
    uint8 mode;                 // S-mode or T-mode
    uint8 state;
} RXinfoDescr;


#define TX_FIFO_THRESHOLD       0x07
#define TX_OCCUPIED_FIF0        32    // Occupied space
#define TX_AVAILABLE_FIFO       32    // Free space
#define TX_FIFO_SIZE            64

#define FIXED_PACKET_LENGTH     0x00
#define INFINITE_PACKET_LENGTH  0x02
#define INFINITE                0
#define FIXED                   1
#define MAX_FIXED_LENGTH        256

#define TX_OK                   0
#define TX_LENGTH_ERROR         1
#define TX_STATE_ERROR          2

// Struct. used to hold information used for TX
typedef struct TXinfoDescr {
    uint16 bytesLeft;           // Bytes left that are to be written to the TX FIFO
    uint8 *pByteIndex;          // Pointer to current position in the byte array
    uint8  format;              // Infinite or fixed length packet mode
    uint8  complete;            // Packet Sendt
} TXinfoDescr;

#define GDO0_DDR  CC1100_OUT_DDR
#define GDO0_PORT CC1100_OUT_PORT
#define GDO0_PIN  CC1100_OUT_IN
#define GDO0_BIT  CC1100_OUT_PIN

#define GDO2_DDR  CC1100_IN_DDR
#define GDO2_PORT CC1100_IN_PORT
#define GDO2_PIN  CC1100_IN_IN
#define GDO2_BIT  CC1100_IN_PIN

#endif
#endif
