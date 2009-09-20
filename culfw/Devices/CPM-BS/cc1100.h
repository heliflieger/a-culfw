/***********************************************************************************
    Filename: cc1100.h

    Copyright 2007 Texas Instruments, Inc.
***********************************************************************************/

#ifndef CC1100_H
#define CC1100_H

#include <avr/io.h>
#include "board.h"

extern void ccInitChip(void);
extern void ccDump(void);
extern uint8_t cc1100_readReg(uint8_t addr);
extern void ccStrobe(uint8_t strobe);

// Configuration Registers
#define CC1100_IOCFG2           0x00        // GDO2 output pin configuration
#define CC1100_IOCFG1           0x01        // GDO1 output pin configuration
#define CC1100_IOCFG0           0x02        // GDO0 output pin configuration
#define CC1100_FIFOTHR          0x03        // RX FIFO and TX FIFO thresholds
#define CC1100_SYNC1            0x04        // Sync word, high byte
#define CC1100_SYNC0            0x05        // Sync word, low byte
#define CC1100_PKTLEN           0x06        // Packet length
#define CC1100_PKTCTRL1         0x07        // Packet automation control
#define CC1100_PKTCTRL0         0x08        // Packet automation control
#define CC1100_ADDR             0x09        // Device address
#define CC1100_CHANNR           0x0A        // Channel number
#define CC1100_FSCTRL1          0x0B        // Frequency synthesizer control
#define CC1100_FSCTRL0          0x0C        // Frequency synthesizer control
#define CC1100_FREQ2            0x0D        // Frequency control word, high byte
#define CC1100_FREQ1            0x0E        // Frequency control word, middle byte
#define CC1100_FREQ0            0x0F        // Frequency control word, low byte
#define CC1100_MDMCFG4          0x10        // Modem configuration
#define CC1100_MDMCFG3          0x11        // Modem configuration
#define CC1100_MDMCFG2          0x12        // Modem configuration
#define CC1100_MDMCFG1          0x13        // Modem configuration
#define CC1100_MDMCFG0          0x14        // Modem configuration
#define CC1100_DEVIATN          0x15        // Modem deviation setting
#define CC1100_MCSM2            0x16        // Main Radio Cntrl State Machine config
#define CC1100_MCSM1            0x17        // Main Radio Cntrl State Machine config
#define CC1100_MCSM0            0x18        // Main Radio Cntrl State Machine config
#define CC1100_FOCCFG           0x19        // Frequency Offset Compensation config
#define CC1100_BSCFG            0x1A        // Bit Synchronization configuration
#define CC1100_AGCCTRL2         0x1B        // AGC control
#define CC1100_AGCCTRL1         0x1C        // AGC control
#define CC1100_AGCCTRL0         0x1D        // AGC control
#define CC1100_WOREVT1          0x1E        // High byte Event 0 timeout
#define CC1100_WOREVT0          0x1F        // Low byte Event 0 timeout
#define CC1100_WORCTRL          0x20        // Wake On Radio control
#define CC1100_FREND1           0x21        // Front end RX configuration
#define CC1100_FREND0           0x22        // Front end TX configuration
#define CC1100_FSCAL3           0x23        // Frequency synthesizer calibration
#define CC1100_FSCAL2           0x24        // Frequency synthesizer calibration
#define CC1100_FSCAL1           0x25        // Frequency synthesizer calibration
#define CC1100_FSCAL0           0x26        // Frequency synthesizer calibration
#define CC1100_RCCTRL1          0x27        // RC oscillator configuration
#define CC1100_RCCTRL0          0x28        // RC oscillator configuration
#define CC1100_FSTEST           0x29        // Frequency synthesizer cal control
#define CC1100_PTEST            0x2A        // Production test
#define CC1100_AGCTEST          0x2B        // AGC test
#define CC1100_TEST2            0x2C        // Various test settings
#define CC1100_TEST1            0x2D        // Various test settings
#define CC1100_TEST0            0x2E        // Various test settings

// Status registers
#define CC1100_PARTNUM          0x30        // Part number
#define CC1100_VERSION          0x31        // Current version number
#define CC1100_FREQEST          0x32        // Frequency offset estimate
#define CC1100_LQI              0x33        // Demodulator estimate for link quality
#define CC1100_RSSI             0x34        // Received signal strength indication
#define CC1100_MARCSTATE        0x35        // Control state machine state
#define CC1100_WORTIME1         0x36        // High byte of WOR timer
#define CC1100_WORTIME0         0x37        // Low byte of WOR timer
#define CC1100_PKTSTATUS        0x38        // Current GDOx status and packet status
#define CC1100_VCO_VC_DAC       0x39        // Current setting from PLL cal module
#define CC1100_TXBYTES          0x3A        // Underflow and # of bytes in TXFIFO
#define CC1100_RXBYTES          0x3B        // Overflow and # of bytes in RXFIFO

// Multi byte memory locations
#define CC1100_PATABLE          0x3E
#define CC1100_TXFIFO           0x3F
#define CC1100_RXFIFO           0x3F

// Definitions for burst/single access to registers
#define CC1100_WRITE_BURST      0x40
#define CC1100_READ_SINGLE      0x80
#define CC1100_READ_BURST       0xC0

// Strobe commands
#define CC1100_SRES             0x30        // Reset chip.
#define CC1100_SFSTXON          0x31        // Enable and calibrate frequency synthesizer (if MCSM0.FS_AUTOCAL=1).
                                            // If in RX/TX: Go to a wait state where only the synthesizer is
                                            // running (for quick RX / TX turnaround).
#define CC1100_SXOFF            0x32        // Turn off crystal oscillator.
#define CC1100_SCAL             0x33        // Calibrate frequency synthesizer and turn it off
                                            // (enables quick start).
#define CC1100_SRX              0x34        // Enable RX. Perform calibration first if coming from IDLE and
                                            // MCSM0.FS_AUTOCAL=1.
#define CC1100_STX              0x35        // In IDLE state: Enable TX. Perform calibration first if
                                            // MCSM0.FS_AUTOCAL=1. If in RX state and CCA is enabled:
                                            // Only go to TX if channel is clear.
#define CC1100_SIDLE            0x36        // Exit RX / TX, turn off frequency synthesizer and exit
                                            // Wake-On-Radio mode if applicable.
#define CC1100_SAFC             0x37        // Perform AFC adjustment of the frequency synthesizer
#define CC1100_SWOR             0x38        // Start automatic RX polling sequence (Wake-on-Radio)
#define CC1100_SPWD             0x39        // Enter power down mode when CSn goes high.
#define CC1100_SFRX             0x3A        // Flush the RX FIFO buffer.
#define CC1100_SFTX             0x3B        // Flush the TX FIFO buffer.
#define CC1100_SWORRST          0x3C        // Reset real time clock.
#define CC1100_SNOP             0x3D        // No operation. May be used to pad strobe commands to two
                                            // bytes for simpler software.


//----------------------------------------------------------------------------------
// Chip Status Byte
//----------------------------------------------------------------------------------

// Bit fields in the chip status byte
#define CC1100_STATUS_CHIP_RDYn_BM             0x80
#define CC1100_STATUS_STATE_BM                 0x70
#define CC1100_STATUS_FIFO_BYTES_AVAILABLE_BM  0x0F

// Chip states
#define CC1100_STATE_IDLE                      0x00
#define CC1100_STATE_RX                        0x10
#define CC1100_STATE_TX                        0x20
#define CC1100_STATE_FSTXON                    0x30
#define CC1100_STATE_CALIBRATE                 0x40
#define CC1100_STATE_SETTLING                  0x50
#define CC1100_STATE_RX_OVERFLOW               0x60
#define CC1100_STATE_TX_UNDERFLOW              0x70


//----------------------------------------------------------------------------------
// Other register bit fields
//----------------------------------------------------------------------------------
#define CC1100_LQI_CRC_OK_BM                   0x80
#define CC1100_LQI_EST_BM                      0x7F

#define CC1100_DEASSERT  SET_BIT( CC1100_CS_PORT, CC1100_CS_PIN )
#define CC1100_ASSERT    CLEAR_BIT( CC1100_CS_PORT, CC1100_CS_PIN )


/**********************************************************************************/
#endif
