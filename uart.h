/*
 * serial.h
 *
 *  Created on: 25.01.2014
 *      Author: Michail Kurochkin
 */

#ifndef UART_H_
#define UART_H_

#include "types.h"

struct uart_regs;
struct uart {
	u8 chip_id;
	int baud_rate;
	u8 fdev_type: 1; // set uart for output printf() or not
	void (*rx_int)(void *, u8);
	void (*udre_int)(void *);
	void (*tx_int)(void *);
	void *priv;
// private:
	struct uart_regs *regs;
};

int usart_init(struct uart *uart);

u8 usart_get_byte(struct uart *uart);
u8 usart_get_byte_blocked(struct uart *uart);
int usart_send_byte(struct uart *uart, u8 byte);
void usart_disable_udre_irq(struct uart *uart);
void usart_enable_udre_irq(struct uart *uart);


#endif /* UART_H_ */
