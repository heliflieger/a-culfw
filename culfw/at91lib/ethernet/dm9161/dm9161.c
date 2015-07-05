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

// components/ethernet/dm9161/dm9161.c

//-----------------------------------------------------------------------------
//         Headers
//-----------------------------------------------------------------------------
#include "dm9161.h"
#include "dm9161_define.h"
#include <pio/pio.h>
#include <rstc/rstc.h>
#include <emac/emac.h>
#include <utility/trace.h>
#include <utility/assert.h>

//-----------------------------------------------------------------------------
//         Definitions
//-----------------------------------------------------------------------------

/// Default max retry count
#define DM9161_RETRY_MAX            100000



//-----------------------------------------------------------------------------
/// Dump all the useful registers
/// \param pDm          Pointer to the Dm9161 instance
//-----------------------------------------------------------------------------
static void DM9161_DumpRegisters(Dm9161 * pDm)
{
    unsigned char phyAddress;
    unsigned int retryMax;
    unsigned int value;

    TRACE_INFO("DM9161_DumpRegisters\n\r");
    ASSERT(pDm, "F: DM9161_DumpRegisters\n\r");

    EMAC_EnableMdio();
    phyAddress = pDm->phyAddress;
    retryMax = pDm->retryMax;

    TRACE_INFO("DM9161 (%d) Registers:\n\r", phyAddress);

    EMAC_ReadPhy(phyAddress, DM9161_BMCR, &value, retryMax);
    TRACE_INFO(" _BMCR   : 0x%X\n\r", value);
    EMAC_ReadPhy(phyAddress, DM9161_BMSR, &value, retryMax);
    TRACE_INFO(" _BMSR   : 0x%X\n\r", value);
    EMAC_ReadPhy(phyAddress, DM9161_ANAR, &value, retryMax);
    TRACE_INFO(" _ANAR   : 0x%X\n\r", value);
    EMAC_ReadPhy(phyAddress, DM9161_ANLPAR, &value, retryMax);
    TRACE_INFO(" _ANLPAR : 0x%X\n\r", value);
    EMAC_ReadPhy(phyAddress, DM9161_ANER, &value, retryMax);
    TRACE_INFO(" _ANER   : 0x%X\n\r", value);
    EMAC_ReadPhy(phyAddress, DM9161_DSCR, &value, retryMax);
    TRACE_INFO(" _DSCR   : 0x%X\n\r", value);
    EMAC_ReadPhy(phyAddress, DM9161_DSCSR, &value, retryMax);
    TRACE_INFO(" _DSCSR  : 0x%X\n\r", value);
    EMAC_ReadPhy(phyAddress, DM9161_10BTCSR, &value, retryMax);
    TRACE_INFO(" _10BTCSR: 0x%X\n\r", value);
    EMAC_ReadPhy(phyAddress, DM9161_PWDOR, &value, retryMax);
    TRACE_INFO(" _PWDOR  : 0x%X\n\r", value);
    EMAC_ReadPhy(phyAddress, DM9161_CONFIGR, &value, retryMax);
    TRACE_INFO(" _CONFIGR: 0x%X\n\r", value);
    EMAC_ReadPhy(phyAddress, DM9161_MDINTR, &value, retryMax);
    TRACE_INFO(" _MDINTR : 0x%X\n\r", value);
    EMAC_ReadPhy(phyAddress, DM9161_RECR, &value, retryMax);
    TRACE_INFO(" _RECR   : 0x%X\n\r", value);
    EMAC_ReadPhy(phyAddress, DM9161_DISCR, &value, retryMax);
    TRACE_INFO(" _DISCR  : 0x%X\n\r", value);
    EMAC_ReadPhy(phyAddress, DM9161_RLSR, &value, retryMax);
    TRACE_INFO(" _RLSR   : 0x%X\n\r", value);

    EMAC_DisableMdio();
}

//-----------------------------------------------------------------------------
/// Find a valid PHY Address ( from 0 to 31 ).
/// Check BMSR register ( not 0 nor 0xFFFF )
/// Return 0xFF when no valid PHY Address found.
/// \param pDm          Pointer to the Dm9161 instance
//-----------------------------------------------------------------------------
static unsigned char DM9161_FindValidPhy(Dm9161 * pDm)
{
    unsigned int  retryMax;
    unsigned int  value=0;
    unsigned char rc;
    unsigned char phyAddress;
    unsigned char cnt;

    TRACE_DEBUG("DM9161_FindValidPhy\n\r");
    ASSERT(pDm, "F: DM9161_FindValidPhy\n\r");

    EMAC_EnableMdio();
    phyAddress = pDm->phyAddress;
    retryMax = pDm->retryMax;

    // Check current phyAddress
    rc = phyAddress;
    if( EMAC_ReadPhy(phyAddress, DM9161_PHYID1, &value, retryMax) == 0 ) {
        TRACE_ERROR("DM9161 PROBLEM\n\r");
    }
    TRACE_DEBUG("_PHYID1  : 0x%X, addr: %d\n\r", value, phyAddress);

    // Find another one
    if (value != DM9161_OUI_MSB) {

        rc = 0xFF;
        for(cnt = 0; cnt < 32; cnt ++) {

            phyAddress = (phyAddress + 1) & 0x1F;
            if( EMAC_ReadPhy(phyAddress, DM9161_PHYID1, &value, retryMax) == 0 ) {
                TRACE_ERROR("DM9161 PROBLEM\n\r");
            }
            TRACE_DEBUG("_PHYID1  : 0x%X, addr: %d\n\r", value, phyAddress);
            if (value == DM9161_OUI_MSB) {

                rc = phyAddress;
                break;
            }
        }
    }
    
    EMAC_DisableMdio();
    if (rc != 0xFF) {

        TRACE_INFO("** Valid PHY Found: %d\n\r", rc);
        EMAC_ReadPhy(phyAddress, DM9161_DSCSR, &value, retryMax);
        TRACE_DEBUG("_DSCSR  : 0x%X, addr: %d\n\r", value, phyAddress);

    }
    return rc;
}


//-----------------------------------------------------------------------------
//         Exported functions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
/// Setup the maximum timeout count of the driver.
/// \param pDm   Pointer to the Dm9161 instance
/// \param toMax Timeout maxmum count.
//-----------------------------------------------------------------------------
void DM9161_SetupTimeout(Dm9161 *pDm, unsigned int toMax)
{
    ASSERT(pDm, "-F- DM9161_SetupTimeout\n\r");

    pDm->retryMax = toMax;
}

//-----------------------------------------------------------------------------
/// Initialize the Dm9161 instance
/// \param pDm          Pointer to the Dm9161 instance
/// \param pEmac        Pointer to the Emac instance for the Dm9161
/// \param phyAddress   The PHY address used to access the PHY
///                     ( pre-defined by pin status on PHY reset )
//-----------------------------------------------------------------------------
void DM9161_Init(Dm9161 *pDm, unsigned char phyAddress)
{
    ASSERT(pDm , "-F- DM9161_Init\n\r");

    pDm->phyAddress = phyAddress;

    // Initialize timeout by default
    pDm->retryMax = DM9161_RETRY_MAX;
}


//-----------------------------------------------------------------------------
/// Issue a SW reset to reset all registers of the PHY
/// Return 1 if successfully, 0 if timeout.
/// \param pDm   Pointer to the Dm9161 instance
//-----------------------------------------------------------------------------
static unsigned char DM9161_ResetPhy(Dm9161 *pDm)
{
    unsigned int retryMax;
    unsigned int bmcr = DM9161_RESET;
    unsigned char phyAddress;
    unsigned int timeout = 10;
    unsigned char ret = 1;

    ASSERT(pDm, "-F- DM9161_ResetPhy");
    TRACE_INFO(" DM9161_ResetPhy\n\r");

    phyAddress = pDm->phyAddress;
    retryMax = pDm->retryMax;

    EMAC_EnableMdio();
    bmcr = DM9161_RESET;
    EMAC_WritePhy(phyAddress, DM9161_BMCR, bmcr, retryMax);

    do {
        EMAC_ReadPhy(phyAddress, DM9161_BMCR, &bmcr, retryMax);
        timeout--;
    } while ((bmcr & DM9161_RESET) && timeout);

    EMAC_DisableMdio();

    if (!timeout) {
        ret = 0;
    }

    return( ret );
}

//-----------------------------------------------------------------------------
/// Do a HW initialize to the PHY ( via RSTC ) and setup clocks & PIOs
/// This should be called only once to initialize the PHY pre-settings.
/// The PHY address is reset status of CRS,RXD[3:0] (the emacPins' pullups).
/// The COL pin is used to select MII mode on reset (pulled up for Reduced MII)
/// The RXDV pin is used to select test mode on reset (pulled up for test mode)
/// The above pins should be predefined for corresponding settings in resetPins
/// The EMAC peripheral pins are configured after the reset done.
/// Return 1 if RESET OK, 0 if timeout.
/// \param pDm         Pointer to the Dm9161 instance
/// \param mck         Main clock setting to initialize clock
/// \param resetPins   Pointer to list of PIOs to configure before HW RESET
///                       (for PHY power on reset configuration latch)
/// \param nbResetPins Number of PIO items that should be configured
/// \param emacPins    Pointer to list of PIOs for the EMAC interface
/// \param nbEmacPins  Number of PIO items that should be configured
//-----------------------------------------------------------------------------
unsigned char DM9161_InitPhy(Dm9161       *pDm,
                             unsigned int mck,
                             const Pin    *pResetPins,
                             unsigned int nbResetPins,
                             const Pin    *pEmacPins,
                             unsigned int nbEmacPins)
{
    unsigned char rc = 1;
    unsigned char phy;

    ASSERT(pDm, "-F- DM9161_InitPhy\n\r");

    // Perform RESET
    TRACE_DEBUG("RESET PHY\n\r");

    if (pResetPins) {

        // Configure PINS
        PIO_Configure(pResetPins, nbResetPins);

        // Execute reset
        RSTC_SetExtResetLength(DM9161_RESET_LENGTH);
        RSTC_ExtReset();

        // Wait for end hardware reset
        while (!RSTC_GetNrstLevel());
    }

    // Configure EMAC runtime pins
    if (rc) {

        PIO_Configure(pEmacPins, nbEmacPins);
        rc = EMAC_SetMdcClock( mck );
        if (!rc) {

            TRACE_ERROR("No Valid MDC clock\n\r");
            return 0;
        }

        // Check PHY Address
        phy = DM9161_FindValidPhy(pDm);
        if (phy == 0xFF) {

            TRACE_ERROR("PHY Access fail\n\r");
            return 0;
        }
        if(phy != pDm->phyAddress) {

            pDm->phyAddress = phy;

            DM9161_ResetPhy(pDm);

        }

    }
    else {

        TRACE_ERROR("PHY Reset Timeout\n\r");
    }

    return rc;
}

//-----------------------------------------------------------------------------
/// Issue a Auto Negotiation of the PHY
/// Return 1 if successfully, 0 if timeout.
/// \param pDm   Pointer to the Dm9161 instance
//-----------------------------------------------------------------------------
unsigned char DM9161_AutoNegotiate(Dm9161 *pDm)
{
    unsigned int retryMax;
    unsigned int value;
    unsigned int phyAnar;
    unsigned int phyAnalpar;
    unsigned int retryCount= 0;
    unsigned char phyAddress;
    unsigned char rc = 1;

    ASSERT(pDm, "-F- DM9161_AutoNegotiate\n\r");
    phyAddress = pDm->phyAddress;
    retryMax = pDm->retryMax;

    EMAC_EnableMdio();

    if (!EMAC_ReadPhy(phyAddress, DM9161_PHYID1, &value, retryMax)) {
        TRACE_ERROR("Pb EMAC_ReadPhy Id1\n\r");
        rc = 0;
        goto AutoNegotiateExit;
    }
    TRACE_DEBUG("ReadPhy Id1 0x%X, addresse: %d\n\r", value, phyAddress);
    if (!EMAC_ReadPhy(phyAddress, DM9161_PHYID2, &phyAnar, retryMax)) {
        TRACE_ERROR("Pb EMAC_ReadPhy Id2\n\r");
        rc = 0;
        goto AutoNegotiateExit;
    }
    TRACE_DEBUG("ReadPhy Id2 0x%X\n\r", phyAnar);

    if( ( value == DM9161_OUI_MSB )
     && ( ((phyAnar>>10)&DM9161_LSB_MASK) == DM9161_OUI_LSB ) ) {

        TRACE_DEBUG("Vendor Number Model = 0x%X\n\r", ((phyAnar>>4)&0x3F));
        TRACE_DEBUG("Model Revision Number = 0x%X\n\r", (phyAnar&0x7));
    }
    else {
        TRACE_ERROR("Problem OUI value\n\r");
    }        

    // Setup control register
    rc  = EMAC_ReadPhy(phyAddress, DM9161_BMCR, &value, retryMax);
    if (rc == 0) {

        goto AutoNegotiateExit;
    }

    value &= ~DM9161_AUTONEG;   // Remove autonegotiation enable
    value &= ~(DM9161_LOOPBACK|DM9161_POWER_DOWN);
    value |=  DM9161_ISOLATE;   // Electrically isolate PHY
    rc = EMAC_WritePhy(phyAddress, DM9161_BMCR, value, retryMax);
    if (rc == 0) {

        goto AutoNegotiateExit;
    }

    // Set the Auto_negotiation Advertisement Register
    // MII advertising for Next page
    // 100BaseTxFD and HD, 10BaseTFD and HD, IEEE 802.3
    phyAnar = DM9161_NP | DM9161_TX_FDX | DM9161_TX_HDX |
              DM9161_10_FDX | DM9161_10_HDX | DM9161_AN_IEEE_802_3;
    rc = EMAC_WritePhy(phyAddress, DM9161_ANAR, phyAnar, retryMax);
    if (rc == 0) {

        goto AutoNegotiateExit;
    }

    // Read & modify control register
    rc  = EMAC_ReadPhy(phyAddress, DM9161_BMCR, &value, retryMax);
    if (rc == 0) {

        goto AutoNegotiateExit;
    }

    value |= DM9161_SPEED_SELECT | DM9161_AUTONEG | DM9161_DUPLEX_MODE;
    rc = EMAC_WritePhy(phyAddress, DM9161_BMCR, value, retryMax);
    if (rc == 0) {

        goto AutoNegotiateExit;
    }

    // Restart Auto_negotiation
    value |=  DM9161_RESTART_AUTONEG;
    value &= ~DM9161_ISOLATE;
    rc = EMAC_WritePhy(phyAddress, DM9161_BMCR, value, retryMax);
    if (rc == 0) {

        goto AutoNegotiateExit;
    }
    TRACE_DEBUG(" _BMCR: 0x%X\n\r", value);

    // Check AutoNegotiate complete
    while (1) {

        rc  = EMAC_ReadPhy(phyAddress, DM9161_BMSR, &value, retryMax);
        if (rc == 0) {

            TRACE_ERROR("rc==0\n\r");
            goto AutoNegotiateExit;
        }
        // Done successfully
        if (value & DM9161_AUTONEG_COMP) {

            TRACE_INFO("AutoNegotiate complete\n\r");
            break;
        }
        // Timeout check
        if (retryMax) {

            if (++ retryCount >= retryMax) {

                DM9161_DumpRegisters(pDm);
                TRACE_FATAL("TimeOut\n\r");
                goto AutoNegotiateExit;
            }
        }
    }

    // Get the AutoNeg Link partner base page
    rc  = EMAC_ReadPhy(phyAddress, DM9161_ANLPAR, &phyAnalpar, retryMax);
    if (rc == 0) {

        goto AutoNegotiateExit;
    }

    // Setup the EMAC link speed
    if ((phyAnar & phyAnalpar) & DM9161_TX_FDX) {

        // set MII for 100BaseTX and Full Duplex
        EMAC_SetLinkSpeed(1, 1);
    }
    else if ((phyAnar & phyAnalpar) & DM9161_10_FDX) {

        // set MII for 10BaseT and Full Duplex
        EMAC_SetLinkSpeed(0, 1);
    }
    else if ((phyAnar & phyAnalpar) & DM9161_TX_HDX) {

        // set MII for 100BaseTX and half Duplex
        EMAC_SetLinkSpeed(1, 0);
    }
    else if ((phyAnar & phyAnalpar) & DM9161_10_HDX) {

        // set MII for 10BaseT and half Duplex
        EMAC_SetLinkSpeed(0, 0);
    }

    // Setup EMAC mode
#if BOARD_EMAC_MODE_RMII != 1
    EMAC_EnableMII();
#else
    EMAC_EnableRMII();
#endif

AutoNegotiateExit:
    EMAC_DisableMdio();
    return rc;
}

//-----------------------------------------------------------------------------
/// Get the Link & speed settings, and automatically setup the EMAC with the
/// settings.
/// Return 1 if link found, 0 if no ethernet link.
/// \param pDm          Pointer to the Dm9161 instance
/// \param applySetting Apply the settings to EMAC interface
//-----------------------------------------------------------------------------
unsigned char DM9161_GetLinkSpeed(Dm9161 *pDm, unsigned char applySetting)
{
    unsigned int retryMax;
    unsigned int stat1;
    unsigned int stat2;
    unsigned char phyAddress;
    unsigned char rc = 1;

    TRACE_DEBUG("DM9161_GetLinkSpeed\n\r");
    ASSERT(pDm, "-F- DM9161_GetLinkSpeed\n\r");

    EMAC_EnableMdio();
    phyAddress = pDm->phyAddress;
    retryMax = pDm->retryMax;

    rc  = EMAC_ReadPhy(phyAddress, DM9161_BMSR, &stat1, retryMax);
    if (rc == 0) {

        goto GetLinkSpeedExit;
    }

    if ((stat1 & DM9161_LINK_STATUS) == 0) {

        TRACE_ERROR("Pb: LinkStat: 0x%x\n\r", stat1);

        rc = 0;
        goto GetLinkSpeedExit;
    }

    if (applySetting == 0) {

        TRACE_ERROR("Pb: applySetting: 0x%x\n\r", applySetting);
        goto GetLinkSpeedExit;
    }

    // Re-configure Link speed
    rc  = EMAC_ReadPhy(phyAddress, DM9161_DSCSR, &stat2, retryMax);
    if (rc == 0) {

        TRACE_ERROR("Pb: rc: 0x%x\n\r", rc);
        goto GetLinkSpeedExit;
    }

    if ((stat1 & DM9161_100BASE_TX_FD) && (stat2 & DM9161_100FDX)) {

        // set Emac for 100BaseTX and Full Duplex
        EMAC_SetLinkSpeed(1, 1);
    }

    if ((stat1 & DM9161_10BASE_T_FD) && (stat2 & DM9161_10FDX)) {

        // set MII for 10BaseT and Full Duplex
        EMAC_SetLinkSpeed(0, 1);
    }

    if ((stat1 & DM9161_100BASE_T4_HD) && (stat2 & DM9161_100HDX)) {

        // set MII for 100BaseTX and Half Duplex
        EMAC_SetLinkSpeed(1, 0);
    }

    if ((stat1 & DM9161_10BASE_T_HD) && (stat2 & DM9161_10HDX)) {

        // set MII for 10BaseT and Half Duplex
        EMAC_SetLinkSpeed(0, 0);
    }

    // Start the EMAC transfers
    TRACE_DEBUG("DM9161_GetLinkSpeed passed\n\r");

GetLinkSpeedExit:
    EMAC_DisableMdio();
    return rc;
}

