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
// private:
	struct uart_regs *regs;
};

int usart_init(struct uart *uart);

u8 usart_get_byte(struct uart *uart);
u8 usart_get_blocked(struct uart *uart);

#endif /* UART_H_ */
