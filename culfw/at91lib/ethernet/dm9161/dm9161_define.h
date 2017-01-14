/* ----------------------------------------------------------------------------
 *         ATMEL Microcontroller Software Support 
 * ----------------------------------------------------------------------------
 * Copyright (c) 2008, Atmel Corporation
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the disclaimer below.
 *
 * Atmel's name may not be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * DISCLAIMER: THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * ----------------------------------------------------------------------------
 */

#ifndef _DM9161_DEFINE_H
#define _DM9161_DEFINE_H

// DAVICOM PHYSICAL LAYER TRANSCEIVER DM9161

//-----------------------------------------------------------------------------
///         Definitions
//-----------------------------------------------------------------------------

#define DM9161_BMCR        0   // Basic Mode Control Register
#define DM9161_BMSR        1   // Basic Mode Status Register
#define DM9161_PHYID1      2   // PHY Idendifier Register 1
#define DM9161_PHYID2      3   // PHY Idendifier Register 2
#define DM9161_ANAR        4   // Auto_Negotiation Advertisement Register
#define DM9161_ANLPAR      5   // Auto_negotiation Link Partner Ability Register
#define DM9161_ANER        6   // Auto-negotiation Expansion Register
#define DM9161_DSCR       16   // Specified Configuration Register
#define DM9161_DSCSR      17   // Specified Configuration and Status Register
#define DM9161_10BTCSR    18   // 10BASE-T Configuration and Satus Register
#define DM9161_PWDOR      19   // Power Down Control Register
#define DM9161_CONFIGR    20   // Specified config Register
#define DM9161_MDINTR     21   // Specified Interrupt Register
#define DM9161_RECR       22   // Specified Receive Error Counter Register
#define DM9161_DISCR      23   // Specified Disconnect Counter Register
#define DM9161_RLSR       24   // Hardware Reset Latch State Register

// Basic Mode Control Register (BMCR)
// Bit definitions: DM9161_BMCR
#define DM9161_RESET             (1 << 15) // 1= Software Reset; 0=Normal Operation
#define DM9161_LOOPBACK          (1 << 14) // 1=loopback Enabled; 0=Normal Operation
#define DM9161_SPEED_SELECT      (1 << 13) // 1=100Mbps; 0=10Mbps
#define DM9161_AUTONEG           (1 << 12) // Auto-negotiation Enable
#define DM9161_POWER_DOWN        (1 << 11) // 1=Power down 0=Normal operation
#define DM9161_ISOLATE           (1 << 10) // 1 = Isolates 0 = Normal operation
#define DM9161_RESTART_AUTONEG   (1 << 9)  // 1 = Restart auto-negotiation 0 = Normal operation
#define DM9161_DUPLEX_MODE       (1 << 8)  // 1 = Full duplex operation 0 = Normal operation
#define DM9161_COLLISION_TEST    (1 << 7)  // 1 = Collision test enabled 0 = Normal operation
//      Reserved                  6 to 0   // Read as 0, ignore on write

// Basic Mode Status Register (BMSR)
// Bit definitions: DM9161_BMSR
#define DM9161_100BASE_T4        (1 << 15) // 100BASE-T4 Capable
#define DM9161_100BASE_TX_FD     (1 << 14) // 100BASE-TX Full Duplex Capable
#define DM9161_100BASE_T4_HD     (1 << 13) // 100BASE-TX Half Duplex Capable
#define DM9161_10BASE_T_FD       (1 << 12) // 10BASE-T Full Duplex Capable
#define DM9161_10BASE_T_HD       (1 << 11) // 10BASE-T Half Duplex Capable
//      Reserved                  10 to 7  // Read as 0, ignore on write
#define DM9161_MF_PREAMB_SUPPR   (1 << 6)  // MII Frame Preamble Suppression
#define DM9161_AUTONEG_COMP      (1 << 5)  // Auto-negotiation Complete
#define DM9161_REMOTE_FAULT      (1 << 4)  // Remote Fault
#define DM9161_AUTONEG_ABILITY   (1 << 3)  // Auto Configuration Ability
#define DM9161_LINK_STATUS       (1 << 2)  // Link Status
#define DM9161_JABBER_DETECT     (1 << 1)  // Jabber Detect
#define DM9161_EXTEND_CAPAB      (1 << 0)  // Extended Capability

// PHY ID Identifier Register
// definitions: DM9161_PHYID1
#define DM9161_PHYID1_OUI         0x606E   // OUI: Organizationally Unique Identifier
#define DM9161_LSB_MASK             0x3F
#define DM9161_ID             0x0181b8a0
#define DM9161_OUI_MSB            0x0181
#define DM9161_OUI_LSB              0x2E

// Auto-negotiation Advertisement Register (ANAR)
// Auto-negotiation Link Partner Ability Register (ANLPAR)
// Bit definitions: DM9161_ANAR, DM9161_ANLPAR
#define DM9161_NP               (1 << 15) // Next page Indication
#define DM9161_ACK              (1 << 14) // Acknowledge
#define DM9161_RF               (1 << 13) // Remote Fault
//      Reserved                12 to 11  // Write as 0, ignore on read
#define DM9161_FCS              (1 << 10) // Flow Control Support
#define DM9161_T4               (1 << 9)  // 100BASE-T4 Support
#define DM9161_TX_FDX           (1 << 8)  // 100BASE-TX Full Duplex Support
#define DM9161_TX_HDX           (1 << 7)  // 100BASE-TX Support
#define DM9161_10_FDX           (1 << 6)  // 10BASE-T Full Duplex Support
#define DM9161_10_HDX           (1 << 5)  // 10BASE-T Support
//      Selector                 4 to 0   // Protocol Selection Bits
#define DM9161_AN_IEEE_802_3      0x0001

// Auto-negotiation Expansion Register (ANER)
// Bit definitions: DM9161_ANER
//      Reserved                15 to 5  // Read as 0, ignore on write
#define DM9161_PDF              (1 << 4) // Local Device Parallel Detection Fault
#define DM9161_LP_NP_ABLE       (1 << 3) // Link Partner Next Page Able
#define DM9161_NP_ABLE          (1 << 2) // Local Device Next Page Able
#define DM9161_PAGE_RX          (1 << 1) // New Page Received
#define DM9161_LP_AN_ABLE       (1 << 0) // Link Partner Auto-negotiation Able

// Specified Configuration Register (DSCR)
// Bit definitions: DM9161_DSCR
#define DM9161_BP4B5B           (1 << 15) // Bypass 4B5B Encoding and 5B4B Decoding
#define DM9161_BP_SCR           (1 << 14) // Bypass Scrambler/Descrambler Function
#define DM9161_BP_ALIGN         (1 << 13) // Bypass Symbol Alignment Function
#define DM9161_BP_ADPOK         (1 << 12) // BYPASS ADPOK
#define DM9161_REPEATER         (1 << 11) // Repeater/Node Mode
#define DM9161_TX               (1 << 10) // 100BASE-TX Mode Control
#define DM9161_FEF              (1 << 9)  // Far end Fault enable
#define DM9161_RMII_ENABLE      (1 << 8)  // Reduced MII Enable
#define DM9161_F_LINK_100       (1 << 7)  // Force Good Link in 100Mbps
#define DM9161_SPLED_CTL        (1 << 6)  // Speed LED Disable
#define DM9161_COLLED_CTL       (1 << 5)  // Collision LED Enable
#define DM9161_RPDCTR_EN        (1 << 4)  // Reduced Power Down Control Enable
#define DM9161_SM_RST           (1 << 3)  // Reset State Machine
#define DM9161_MFP_SC           (1 << 2)  // MF Preamble Suppression Control
#define DM9161_SLEEP            (1 << 1)  // Sleep Mode
#define DM9161_RLOUT            (1 << 0)  // Remote Loopout Control

// Specified Configuration and Status Register (DSCSR)
// Bit definitions: DM9161_DSCSR
#define DM9161_100FDX           (1 << 15) // 100M Full Duplex Operation Mode
#define DM9161_100HDX           (1 << 14) // 100M Half Duplex Operation Mode
#define DM9161_10FDX            (1 << 13) // 10M Full Duplex Operation Mode
#define DM9161_10HDX            (1 << 12) // 10M Half Duplex Operation Mode

// 10BASE-T Configuration/Status (10BTCSR)
// Bit definitions: DM9161_10BTCSR
//      Reserved                18 to 15  // Read as 0, ignore on write
#define DM9161_LP_EN            (1 << 14) // Link Pulse Enable
#define DM9161_HBE              (1 << 13) // Heartbeat Enable
#define DM9161_SQUELCH          (1 << 12) // Squelch Enable
#define DM9161_JABEN            (1 << 11) // Jabber Enable
#define DM9161_10BT_SER         (1 << 10) // 10BASE-T GPSI Mode
//      Reserved                 9 to  1  // Read as 0, ignore on write
#define DM9161_POLR             (1 << 0)  // Polarity Reversed

// Specified Interrupt Register
// Bit definitions: DM9161_MDINTR
#define DM9161_INTR_PEND        (1 << 15) // Interrupt Pending
//      Reserved                14 to 12  // Reserved
#define DM9161_FDX_MASK         (1 << 11) // Full-duplex Interrupt Mask
#define DM9161_SPD_MASK         (1 << 10) // Speed Interrupt Mask
#define DM9161_LINK_MASK        (1 << 9)  // Link Interrupt Mask
#define DM9161_INTR_MASK        (1 << 8)  // Master Interrupt Mask
//      Reserved                 7 to 5   // Reserved
#define DM9161_FDX_CHANGE       (1 << 4)  // Duplex Status Change Interrupt
#define DM9161_SPD_CHANGE       (1 << 3)  // Speed Status Change Interrupt
#define DM9161_LINK_CHANGE      (1 << 2)  // Link Status Change Interrupt
//      Reserved                      1   // Reserved
#define DM9161_INTR_STATUS      (1 << 0)  // Interrupt Status

#endif // #ifndef _DM9161_DEFINE_H

