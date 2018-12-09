#include "board.h"
#ifdef HAS_WIZNET

#include <DHCP/dhcp.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include <hal.h>
#include <socket.h>
#include <stdint.h>
#include <stdio.h>
#include <utility/trace.h>
#include <wizchip_conf.h>

#include "delay.h"
#include "display.h"
#include "ethernet.h"
#include "fncollection.h"
#include "spi.h"
#include "stringfunc.h"
#include "ttydata.h"

//////////////////////////////////////////////////
// Socket & Port number definition for Examples //
//////////////////////////////////////////////////

#define SOCK_DHCP 3

////////////////////////////////////////////////
// Shared Buffer Definition for Loopback test //
////////////////////////////////////////////////
#define DATA_BUF_SIZE 1024
uint8_t gDATABUF[DATA_BUF_SIZE];

///////////////////////////
// Network Configuration //
///////////////////////////
wiz_NetInfo gWIZNETINFO = {
  .mac = {0x00, 0x80, 0x41, 0xbb, 0xcd, 0x1f},
  .ip = {192, 168, 69, 80},
  .sn = {255, 255, 255, 0},
  .gw = {192, 168, 69, 254},
  .dns = {0, 0, 0, 0},
  .dhcp = NETINFO_STATIC
};
//NETINFO_DHCP NETINFO_STATIC

rb_t NET_Tx_Buffer;

uint8_t run_user_applications = 0;

void wizchip_select(void) {
  my_delay_us(1);
  HAL_GPIO_WritePin( WIZNET_CS_GPIO, _BV(WIZNET_CS_PIN), GPIO_PIN_RESET);
}

void wizchip_deselect(void) {
  my_delay_us(1);
  HAL_GPIO_WritePin( WIZNET_CS_GPIO, _BV(WIZNET_CS_PIN), GPIO_PIN_SET);
}

uint8_t wizchip_read() {
  return spi2_send( 0xff );
}

void wizchip_write(uint8_t wb) {
  spi2_send( wb );
}

void my_ip_assign(void) {
  getIPfromDHCP(gWIZNETINFO.ip);
  getGWfromDHCP(gWIZNETINFO.gw);
  getSNfromDHCP(gWIZNETINFO.sn);
  getDNSfromDHCP(gWIZNETINFO.dns);
  gWIZNETINFO.dhcp = NETINFO_DHCP;

  /* Network initialization */
  ctlnetwork(CN_SET_NETINFO, (void*) &gWIZNETINFO);

  TRACE_INFO_WP("assigned IP from DHCP\r\n");
}

/************************************
 * @ brief Call back for ip Conflict
 ************************************/
void my_ip_conflict(void) {
  TRACE_INFO_WP("CONFLICT IP from DHCP\r\n");
}


uint8_t check_Net_MAC() {
  uint8_t buf[3] = {0x00, 0x80, 0x41};

  ctlnetwork(CN_GET_NETINFO, (void*) &gWIZNETINFO);

  if(memcmp(buf,gWIZNETINFO.mac,3)) {
    return 0;
  }

  return 1;

}


static void Display_Net_Conf()
{

  uint8_t tmpstr[6] = {0,};
  ctlnetwork(CN_GET_NETINFO, (void*) &gWIZNETINFO);

  // Display Network Information
  ctlwizchip(CW_GET_ID,(void*)tmpstr);

  if(gWIZNETINFO.dhcp == NETINFO_DHCP) {
    TRACE_INFO_WP("\r\n===== %s NET CONF : DHCP =====\n\r",(char*)tmpstr);
  } else {
    TRACE_INFO_WP("\r\n===== %s NET CONF : Static =====\n\r",(char*)tmpstr);
  }
  TRACE_INFO_WP(" MAC : %02X:%02X:%02X:%02X:%02X:%02X\n\r", gWIZNETINFO.mac[0], gWIZNETINFO.mac[1], gWIZNETINFO.mac[2], gWIZNETINFO.mac[3], gWIZNETINFO.mac[4], gWIZNETINFO.mac[5]);
  TRACE_INFO_WP(" IP : %d.%d.%d.%d\n\r", gWIZNETINFO.ip[0], gWIZNETINFO.ip[1], gWIZNETINFO.ip[2], gWIZNETINFO.ip[3]);
  TRACE_INFO_WP(" GW : %d.%d.%d.%d\n\r", gWIZNETINFO.gw[0], gWIZNETINFO.gw[1], gWIZNETINFO.gw[2], gWIZNETINFO.gw[3]);
  TRACE_INFO_WP(" SN : %d.%d.%d.%d\n\r", gWIZNETINFO.sn[0], gWIZNETINFO.sn[1], gWIZNETINFO.sn[2], gWIZNETINFO.sn[3]);
  TRACE_INFO_WP("=======================================\n\r");
}

void                             // EEPROM Read IP
erip(void *ip, uint8_t *addr)
{
  uint8_t *p = ip;
  p[0] = erb(addr);
  p[1] = erb(addr+1);
  p[2] = erb(addr+2);
  p[3] = erb(addr+3);
}

void
ethernet_reset(void)
{
  char buf[21];


  buf[1] = 'i';
  buf[2] = 'd'; strcpy_P(buf+3, PSTR("1"));             write_eeprom(buf);//DHCP
  buf[2] = 'a'; strcpy_P(buf+3, PSTR("192.168.0.244")); write_eeprom(buf);//IP
  buf[2] = 'n'; strcpy_P(buf+3, PSTR("255.255.255.0")); write_eeprom(buf);
  buf[2] = 'g'; strcpy_P(buf+3, PSTR("192.168.0.1"));   write_eeprom(buf);//GW
  buf[2] = 'p'; strcpy_P(buf+3, PSTR("2323"));          write_eeprom(buf);
  buf[2] = 'N'; strcpy_P(buf+3, PSTR("0.0.0.0"));       write_eeprom(buf);//==GW
  buf[2] = 'o'; strcpy_P(buf+3, PSTR("00"));            write_eeprom(buf);//GMT

  uint32_t fserial = flash_serial();

  buf[2] = 'm'; strcpy_P(buf+3, PSTR("008041"));

  tohex((uint8_t)(fserial>>16 & 0xff), (uint8_t*)buf+9);
  tohex((uint8_t)(fserial>>8 & 0xff), (uint8_t*)buf+11);
  tohex((uint8_t)(fserial & 0xff), (uint8_t*)buf+13);

  write_eeprom(buf);//MAC

}

void ethernet_init(void) {
  uint8_t memsize[2][8] = { { 1, 1, 1, 1, 1, 1, 1, 1 }, { 1, 1, 1, 1, 1, 1, 1, 1 } };
  
  hal_wiznet_Init();

  reg_wizchip_cs_cbfunc(wizchip_select, wizchip_deselect);
  reg_wizchip_spi_cbfunc(wizchip_read, wizchip_write);

  //reg_wizchip_spiburst_cbfunc(spi2_transmit_burst,spi2_transmit_burst);

  /* wizchip initialize*/
  if (ctlwizchip(CW_INIT_WIZCHIP, (void*) memsize) == -1) {
    TRACE_INFO_WP("WIZCHIP Initialized fail.\n\r");
  } else {
    TRACE_INFO_WP("WIZCHIP Initialized success.\n\r");
  }
  DNL();


  gWIZNETINFO.mac[0] = erb(EE_MAC_ADDR+0);
  gWIZNETINFO.mac[1] = erb(EE_MAC_ADDR+1);
  gWIZNETINFO.mac[2] = erb(EE_MAC_ADDR+2);
  gWIZNETINFO.mac[3] = erb(EE_MAC_ADDR+3);
  gWIZNETINFO.mac[4] = erb(EE_MAC_ADDR+4);
  gWIZNETINFO.mac[5] = erb(EE_MAC_ADDR+5);

  erip(gWIZNETINFO.ip, EE_IP4_ADDR);
  erip(gWIZNETINFO.gw, EE_IP4_GATEWAY);
  erip(gWIZNETINFO.sn, EE_IP4_NETMASK);

  if(erb(EE_USE_DHCP)) {
    gWIZNETINFO.dhcp = NETINFO_DHCP;
  } else {
    gWIZNETINFO.dhcp = NETINFO_STATIC;
  }
  
  // fix NetInfo and set
  ctlnetwork(CN_SET_NETINFO, (void*) &gWIZNETINFO);

  /* DHCP client Initialization */
  if(gWIZNETINFO.dhcp == NETINFO_DHCP) {
    DHCP_init(SOCK_DHCP, gDATABUF);
    // if you want different action instead default ip assign, update, conflict.
    // if cbfunc == 0, act as default.
    reg_dhcp_cbfunc(my_ip_assign, my_ip_assign, my_ip_conflict);
    run_user_applications = 0; // flag for running user's code
  } else {
    Display_Net_Conf();
    run_user_applications = 1; 
  }
}


int32_t rxtx_0() {
  int32_t ret;
  uint16_t size = 0, sentsize=0;

  if((size = getSn_RX_RSR(0)) > 0) {
    sentsize = TTY_BUFSIZE - TTY_Rx_Buffer.nbytes;
    if(size > sentsize) size = sentsize;
    ret = recv(0, gDATABUF, size);
    if(ret <= 0) return ret;

    sentsize = 0;
    while(size != sentsize)
      rb_put(&TTY_Rx_Buffer, gDATABUF[sentsize++]);

    input_handle_func(DISPLAY_TCP);
  }
    
  size = 0;
  while (NET_Tx_Buffer.nbytes) {
    gDATABUF[size++] = rb_get(&NET_Tx_Buffer);
  }

  sentsize = 0;
  while(size != sentsize) {
    ret = send(0, gDATABUF+sentsize,size-sentsize);
    if(ret < 0) {
      close(0);
      return ret;
    }
    sentsize += ret; // Don't care SOCKERR_BUSY, because it is zero.
  }

  return 1;
}

#define NET_BUF_SIZE  64
#define NET_COUNT     CDC_COUNT-1

uint8_t net_rx_buffer[NET_COUNT][NET_BUF_SIZE];
volatile uint8_t net_rx_size[NET_COUNT];

int32_t rxtx(uint8_t net_num) {
  int32_t ret;
  uint16_t size = 0, sentsize=0;
  
  if(!net_rx_size[net_num] && ((size = getSn_RX_RSR(net_num+1)) > 0)) {

    sentsize = NET_BUF_SIZE;
    if(size > sentsize) size = sentsize;

    ret = recv(net_num+1, net_rx_buffer[net_num], size);
    if(ret <= 0) return ret;

    net_rx_size[net_num] = size;
    TRACE_DEBUG_WP("%d:NET_UART_TRANSMIT: %d\r\n",net_num+1, size);
    HAL_UART_Write(net_num, &net_rx_buffer[net_num][0], net_rx_size[net_num]);
  }
  return 1;
}

// TCP Server - does keep the sockets listening
//
int32_t tcp_server(uint8_t sn, uint16_t port) {
  int32_t ret;
  uint8_t destip[4];
  uint16_t destport;
  switch(getSn_SR(sn)) {
  case SOCK_ESTABLISHED :
    if(getSn_IR(sn) & Sn_IR_CON) {
      getSn_DIPR(sn, destip);
      destport = getSn_DPORT(sn);
      TRACE_INFO_WP("%d:Connected - %d.%d.%d.%d : %d\r\n",sn, destip[0], destip[1], destip[2], destip[3], destport);
      setSn_IR(sn,Sn_IR_CON);
    }

    if (sn == 0) {
      return rxtx_0();
    } else if(sn <= NET_COUNT) {
      return rxtx(sn-1);
    }

    break;
  case SOCK_CLOSE_WAIT :
    TRACE_INFO_WP("%d:CloseWait\r\n",sn);
    if((ret=disconnect(sn)) != SOCK_OK) return ret;
    TRACE_INFO_WP("%d:Socket closed\r\n",sn);
    break;
  case SOCK_INIT :
    TRACE_INFO_WP("%d:Listen, TCP server, port [%d]\r\n",sn, port);
    if( (ret = listen(sn)) != SOCK_OK) return ret;
    break;
  case SOCK_CLOSED:
    TRACE_INFO_WP("%d:TCP server start\r\n",sn);
    if((ret=socket(sn, Sn_MR_TCP, port, 0x00)) != sn)
      //if((ret=socket(sn, Sn_MR_TCP, port, Sn_MR_ND)) != sn)
      return ret;
    TRACE_INFO_WP("%d:Socket opened\r\n",sn);
    break;
  default:
    break;
  }

  return 1;
}

void Ethernet_Task(void) {

  if(gWIZNETINFO.dhcp == NETINFO_DHCP) {
    // make and keep DHCP happy ...
    switch(DHCP_run()) {
    case DHCP_IP_ASSIGN:
    case DHCP_IP_CHANGED:
      /* If this block empty, act with default_ip_assign & default_ip_update */
      //
      // This example calls my_ip_assign in the two case.
      //
      // Add to ...
      //
      break;
    case DHCP_IP_LEASED:
      //
      // TODO: insert user's code here
      if (!run_user_applications)
        Display_Net_Conf();

      run_user_applications = 1;
      //
      break;
    case DHCP_FAILED:
      /* ===== Example pseudo code ===== */
      // The below code can be replaced your code or omitted.
      // if omitted, retry to process DHCP
      /*
      my_dhcp_retry++;
      if(my_dhcp_retry > MY_MAX_DHCP_RETRY) {
	gWIZNETINFO.dhcp = NETINFO_STATIC;
	DHCP_stop(); // if restart, recall DHCP_init()
	printf(">> DHCP %d Failed\r\n", my_dhcp_retry);
	Net_Conf();
	Display_Net_Conf(); // print out static netinfo to serial
	my_dhcp_retry = 0;
      }
      */
      break;
      /*
    case DHCP_RUNNING:
      DC('R');
      break;
    case DHCP_STOPPED:
      DC('S');
      break;
      */
    default:
      break;
    }
  }

  if (!run_user_applications)
    return;

  tcp_server( 0, 2323 );
#if NET_COUNT > 0
  tcp_server( 1, 2324 );
#endif
#if NET_COUNT > 1
  tcp_server( 2, 2325 );
#endif
}

void NET_Receive_next(uint8_t net_num) {
  if(net_num < NET_COUNT) {
    net_rx_size[net_num] = 0;
  }
}

void Net_Write(uint8_t *data, uint16_t size,  uint8_t socket) {
  uint8_t ret;

  if(getSn_SR(socket) == SOCK_ESTABLISHED) {
    ret = send(socket,data,size);
    if(ret < 0) {
      close(socket);
    }
  }
}

#endif

