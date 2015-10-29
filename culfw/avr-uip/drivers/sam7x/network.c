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
#include <emac/emac.h>
#include <ethernet/dm9161/dm9161.h>
#include <string.h>
#include <utility/trace.h>
#include <utility/assert.h>
#include "uip.h"
#include "uip_arp.h"
#include "fncollection.h"

#include "led.h"

//#include "network.h"


#include "uip.h"

/// EMAC power control pin
#if !defined(BOARD_EMAC_POWER_ALWAYS_ON)
static const Pin emacPwrDn[] = {BOARD_EMAC_PIN_PWRDN};
#endif

/// The PINs' on PHY reset
//static const Pin emacRstPins[] = {BOARD_EMAC_RST_PINS};

/// The PINs for EMAC
static const Pin emacPins[] = {BOARD_EMAC_RUN_PINS};

/// PHY address
#define EMAC_PHY_ADDR             31


/// The DM9161 driver instance
static Dm9161 gDm9161;

#define BUF ((struct uip_eth_hdr *)uip_buf)

static struct uip_eth_addr MacAddress;

static int  linkstate = 0;
//-----------------------------------------------------------------------------
/// Emac interrupt handler
//-----------------------------------------------------------------------------
static void ISR_Emac(void)
{
    EMAC_Handler();
}

/*---------------------------------------------------------------------------*/
void
network_init(void)
{
    Dm9161       *pDm = &gDm9161;

    MacAddress.addr[0] = erb(EE_MAC_ADDR+0);
    MacAddress.addr[1] = erb(EE_MAC_ADDR+1);
    MacAddress.addr[2] = erb(EE_MAC_ADDR+2);
    MacAddress.addr[3] = erb(EE_MAC_ADDR+3);
    MacAddress.addr[4] = erb(EE_MAC_ADDR+4);
    MacAddress.addr[5] = erb(EE_MAC_ADDR+5);

    // Display MAC & IP settings
    TRACE_INFO(" - MAC %x:%x:%x:%x:%x:%x\n\r",
           MacAddress.addr[0], MacAddress.addr[1], MacAddress.addr[2],
           MacAddress.addr[3], MacAddress.addr[4], MacAddress.addr[5]);

    // clear PHY power down mode
    PIO_Configure(emacPwrDn, 1);

    // Init EMAC driver structure
    EMAC_Init(AT91C_ID_EMAC, MacAddress.addr, EMAC_CAF_ENABLE, EMAC_NBC_DISABLE);

    // Setup EMAC buffers and interrupts
    AIC_ConfigureIT(AT91C_ID_EMAC, AT91C_AIC_SRCTYPE_INT_HIGH_LEVEL, ISR_Emac);
    AIC_EnableIT(AT91C_ID_EMAC);

    // Init DM9161 driver
    DM9161_Init(pDm, EMAC_PHY_ADDR);

    // PHY initialize
    //if (!DM9161_InitPhy(pDm, BOARD_MCK,
    //                        emacRstPins, PIO_LISTSIZE(emacRstPins),
    //                        emacPins, PIO_LISTSIZE(emacPins))) {

    if (!DM9161_InitPhy(pDm, BOARD_MCK,
                        0, 0,
                        emacPins, PIO_LISTSIZE(emacPins))) {

    	TRACE_INFO("P: PHY Initialize ERROR!\n\r");
        return;
    }

}

//-----------------------------------------------------------------------------
/// Read for EMAC device
//-----------------------------------------------------------------------------
unsigned int
network_read(void)
{
    unsigned int pkt_len = 0;
    unsigned char error = EMAC_Poll( (unsigned char*)uip_buf, UIP_CONF_BUFFER_SIZE, &pkt_len);
    if (error == EMAC_RX_FRAME_SIZE_TOO_SMALL)
    {
        EMAC_Discard_Fragments();
        pkt_len = 0;
    }
    else if(error != EMAC_RX_OK) 
    {
        pkt_len = 0;
    }

    return pkt_len;
}

//-----------------------------------------------------------------------------
/// Send to EMAC device
//-----------------------------------------------------------------------------
void network_send(void)
{
    unsigned char emac_rc;

    emac_rc = EMAC_Send( (void*)uip_buf, uip_len, (EMAC_TxCallback)0);
    if (emac_rc != EMAC_TX_OK) {

        TRACE_ERROR("E: Send, rc 0x%x\n\r", emac_rc);
    }

}

/*---------------------------------------------------------------------------*/

void network_get_MAC(uint8_t* macaddr)
{

}

void network_set_MAC(uint8_t* macaddr)
{

}

void network_set_led(uint16_t led) {

}


void ethernet_process(void) {


	if(linkstate == 0) {
		Dm9161       *pDm = &gDm9161;
		if( DM9161_GetLinkSpeed(pDm, 1) != 0 ) {

			TRACE_INFO("P: Link detected\n\r");
			linkstate=1;
			LED2_ON();
			// Auto Negotiate
			if (!DM9161_AutoNegotiate(pDm)) {

				TRACE_INFO("P: Auto Negotiate ERROR!\n\r");
				return;
			}
		}
	} else {


		uip_len = network_read();

		if(uip_len > 0) {

			if(BUF->type == htons(UIP_ETHTYPE_IP)){
				uip_arp_ipin();
				uip_input();
				if(uip_len > 0) {
					uip_arp_out();
					network_send();
				}

			} else if(BUF->type == htons(UIP_ETHTYPE_ARP)){

				uip_arp_arpin();
				if(uip_len > 0){
					network_send();
				}
			}
		}
	}

}

void interface_periodic(void) {
}
