/*
 * Simple common network interface that all network drivers should implement.
 */

#ifndef __NETWORK_H__
#define __NETWORK_H__

/*Initialize the network*/
void network_init(void);

/*Read from the network, returns number of read bytes*/
unsigned int network_read(void);

/*Send using the network*/
void network_send(void);

/*Sets the MAC address of the device*/
void network_set_MAC(uint8_t* mac);

/*Gets the MAC address of the device*/
void network_get_MAC(uint8_t* mac);

void ethernet_process(void);

/*Sets the LED*/
void network_set_led(uint16_t led);

void interface_periodic(void);

#endif /* __NETWORK_H__ */
