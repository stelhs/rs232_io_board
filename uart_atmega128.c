/*
 * uart_atmega128.c
 *
 *  Created on: 06.07.2016
 *      Author: Michail Kurochkin
 */

#include <avr/wdt.h>
#include <stdio.h>
#include "types.h"
#include "uart.h"

static struct uart_regs {
	volatile u8 *udr;
	volatile u8 *ucsra;
	volatile u8 *ucsrb;
	volatile u8 *ucsrc;

	volatile u8 *ubrrh;
	volatile u8 *ubrrl;
} uart_ports[] = {
		{
				.udr = &UDR0,
				.ucsra = &UCSR0A,
				.ucsrb = &UCSR0B,
				.ucsrc = &UCSR0C,
				.ubrrh = &UBRR0H,
				.ubrrl = &UBRR0L,
		},
		{
				.udr = &UDR1,
				.ucsra = &UCSR1A,
				.ucsrb = &UCSR1B,
				.ucsrc = &UCSR1C,
				.ubrrh = &UBRR1H,
				.ubrrl = &UBRR1L,
		},
};

static struct uart_regs *console_regs = NULL;

int usart_send_byte(struct uart *uart, u8 byte)
{
	while (!(*uart->regs->ucsra & (1 << UDRE0)));
	*uart->regs->udr = byte;
	return 0;
}

u8 usart_get_byte(struct uart *uart)
{
	if ((*uart->regs->ucsra & (1 << RXC0)))
		return *uart->regs->udr;
	return 0;
}

u8 usart_get_blocked(struct uart *uart)
{
	while (!(*uart->regs->ucsra & (1 << RXC0)));
	return *uart->regs->udr;
}

static int serial_out(char byte)
{

	while(!(*console_regs->ucsra & (1 << UDRE0)));
	*console_regs->udr = byte;
	return 0;
}

static char serial_in(void)
{
	while(!(*console_regs->ucsra & (1 << RXC0)));
	return *console_regs->udr;
}


int usart_init(struct uart *uart)
{
	u16 ubrr;
	if (uart->chip_id < 0 || uart->chip_id > 1)
		return -EINVAL;

	uart->regs = uart_ports + uart->chip_id;
	ubrr = F_CPU / 16 / uart->baud_rate - 1;

	*uart->regs->ubrrh = (u8) (ubrr >> 8);
	*uart->regs->ubrrl = (u8) ubrr;
	*uart->regs->ucsrb = (1 << RXEN0) | (1 << TXEN0);
	*uart->regs->ucsrc = (3 << UCSZ0) | (3 << UCSZ0);

	if (uart->fdev_type) {
		console_regs = uart->regs;
		fdevopen((void *) serial_out, (void *) serial_in);
	}
	return 0;
}
