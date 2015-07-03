/*
 * Copyright (c) 2001, Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 * $Id: tapdev.c,v 1.8 2006/06/07 08:39:58 adam Exp $
 */

#define UIP_DRIPADDR0   192
#define UIP_DRIPADDR1   168
#define UIP_DRIPADDR2   0
#define UIP_DRIPADDR3   1

#include <board.h>
#include <pio/pio.h>
#include <aic/aic.h>
#include <dbgu/dbgu.h>
//#include <usart/usart.h>
#include <emac/emac.h>
#include <ethernet/dm9161/dm9161.h>
#include <string.h>
#include <utility/trace.h>
#include <utility/assert.h>
#include "uip.h"
#include "uip_arp.h"
#include "tapdev.h"


#include "uip.h"

/// EMAC power control pin
#if !defined(BOARD_EMAC_POWER_ALWAYS_ON)
static const Pin emacPwrDn[] = {BOARD_EMAC_PIN_PWRDN};
#endif

/// The PINs' on PHY reset
static const Pin emacRstPins[] = {BOARD_EMAC_RST_PINS};

/// The PINs for EMAC
static const Pin emacPins[] = {BOARD_EMAC_RUN_PINS};

/// PHY address
#define EMAC_PHY_ADDR             31


/// The DM9161 driver instance
static Dm9161 gDm9161;

//-----------------------------------------------------------------------------
/// Emac interrupt handler
//-----------------------------------------------------------------------------
static void ISR_Emac(void)
{
    EMAC_Handler();
}

/*---------------------------------------------------------------------------*/
void
tapdev_init(void)
{
    Dm9161       *pDm = &gDm9161;
    unsigned int errCount = 0;

    TRACE_CONFIGURE(DBGU_STANDARD, 115200, BOARD_MCK);
    printf("-- Basic EMAC uIP Project %s --\n\r", SOFTPACK_VERSION);
    printf("-- %s\n\r", BOARD_NAME);
    printf("-- Compiled: %s %s --\n\r", __DATE__, __TIME__);

    // Display MAC & IP settings
    printf(" - MAC %x:%x:%x:%x:%x:%x\n\r",
           MacAddress.addr[0], MacAddress.addr[1], MacAddress.addr[2],
           MacAddress.addr[3], MacAddress.addr[4], MacAddress.addr[5]);

#if !defined(UIP_DHCP_on)
    printf(" - Host IP  %d.%d.%d.%d\n\r",
           HostIpAddress[0], HostIpAddress[1], HostIpAddress[2], HostIpAddress[3]);
    printf(" - Router IP  %d.%d.%d.%d\n\r",
           RoutIpAddress[0], RoutIpAddress[1], RoutIpAddress[2], RoutIpAddress[3]);
    printf(" - Net Mask  %d.%d.%d.%d\n\r",
           NetMask[0], NetMask[1], NetMask[2], NetMask[3]);
#endif

#if !defined(BOARD_EMAC_POWER_ALWAYS_ON)
    // clear PHY power down mode
    PIO_Configure(emacPwrDn, 1);
#endif

    // Init EMAC driver structure
    EMAC_Init(AT91C_ID_EMAC, MacAddress.addr, EMAC_CAF_ENABLE, EMAC_NBC_DISABLE);

    // Setup EMAC buffers and interrupts
    AIC_ConfigureIT(AT91C_ID_EMAC, AT91C_AIC_SRCTYPE_INT_HIGH_LEVEL, ISR_Emac);
    AIC_EnableIT(AT91C_ID_EMAC);

    // Init DM9161 driver
    DM9161_Init(pDm, EMAC_PHY_ADDR);

    // PHY initialize
    if (!DM9161_InitPhy(pDm, BOARD_MCK,
                        emacRstPins, PIO_LISTSIZE(emacRstPins),
                        emacPins, PIO_LISTSIZE(emacPins))) {

        printf("P: PHY Initialize ERROR!\n\r");
        return;
    }

    // Auto Negotiate
    if (!DM9161_AutoNegotiate(pDm)) {

        printf("P: Auto Negotiate ERROR!\n\r");
        return;
    }

    while( DM9161_GetLinkSpeed(pDm, 1) == 0 ) {

        errCount++;
    }
    printf("P: Link detected\n\r");


}

//-----------------------------------------------------------------------------
/// Read for EMAC device
//-----------------------------------------------------------------------------
unsigned int
tapdev_read(void)
{
    unsigned int pkt_len = 0;
    if( EMAC_RX_OK != EMAC_Poll( (unsigned char*)uip_buf,
                                  UIP_CONF_BUFFER_SIZE,
                                  &pkt_len) ) {

        pkt_len = 0;
    }

    return pkt_len;
}

//-----------------------------------------------------------------------------
/// Send to EMAC device
//-----------------------------------------------------------------------------
void tapdev_send(void)
{
    unsigned char emac_rc;
    unsigned long i;

    TRACE_INFO_WP("w ");
        for( i=0;i<uip_len;i++) {
        	TRACE_INFO_WP("%02x ",uip_buf[i]);
        }
        TRACE_INFO_WP("\n\r");

    emac_rc = EMAC_Send( (void*)uip_buf, uip_len, (EMAC_TxCallback)0);
    if (emac_rc != EMAC_TX_OK) {

        TRACE_ERROR("E: Send, rc 0x%x\n\r", emac_rc);
    }
}

/*---------------------------------------------------------------------------*/

