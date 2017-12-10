/*
 * cdc_uart.h
 *
 *  Created on: 01.12.2016
 *      Author: Telekatz
 */

#ifndef CDC_UART_H_
#define CDC_UART_H_

void cdc_uart_init(void);
void cdc_uart_task(void);
void cdc_uart_func(char *in);
void EE_write_baud(uint8_t num, uint32_t baud);

#endif /* CDC_UART_H_ */
