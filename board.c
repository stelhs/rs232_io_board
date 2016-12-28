/*
 * board.c
 *
 *  Created on: 29.12.2016
 *      Author: Michail Kurochkin
 */

#include <stdio.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "board.h"
#include "types.h"
#include "idle.h"
#include "gpio.h"
#include "gpio_debouncer.h"
#include "leds.h"
#include "config.h"
#include "uart.h"
#include "eeprom_fs.h"

struct gpio gpio_list[] = {
	/* List inputs */
	{ // 0: input 1
		.direction_addr = (u8 *) &DDRA,
		.port_addr = (u8 *) &PORTA,
		.pin_addr = (u8 *) &PINA,
		.pin = 0,
		.direction = GPIO_INPUT,
		.pull_up = 1
	},
	{ // 1: input 2
		.direction_addr = (u8 *) &DDRA,
		.port_addr = (u8 *) &PORTA,
		.pin_addr = (u8 *) &PINA,
		.pin = 1,
		.direction = GPIO_INPUT,
		.pull_up = 1
	},
	{ // 2: input 3
		.direction_addr = (u8 *) &DDRF,
		.port_addr = (u8 *) &PORTF,
		.pin_addr = (u8 *) &PINF,
		.pin = 0,
		.direction = GPIO_INPUT,
		.pull_up = 1
	},
	{ // 3: input 4
		.direction_addr = (u8 *) &DDRF,
		.port_addr = (u8 *) &PORTF,
		.pin_addr = (u8 *) &PINF,
		.pin = 2,
		.direction = GPIO_INPUT,
		.pull_up = 1
	},
	{ // 4: input 5
		.direction_addr = (u8 *) &DDRA,
		.port_addr = (u8 *) &PORTA,
		.pin_addr = (u8 *) &PINA,
		.pin = 5,
		.direction = GPIO_INPUT,
		.pull_up = 1
	},
	{ // 5: input 7
		.direction_addr = (u8 *) &DDRF,
		.port_addr = (u8 *) &PORTF,
		.pin_addr = (u8 *) &PINF,
		.pin = 1,
		.direction = GPIO_INPUT,
		.pull_up = 1
	},
	{ // 6: input 8
		.direction_addr = (u8 *) &DDRF,
		.port_addr = (u8 *) &PORTF,
		.pin_addr = (u8 *) &PINF,
		.pin = 3,
		.direction = GPIO_INPUT,
		.pull_up = 1
	},
	{ // 7: input 9
		.direction_addr = (u8 *) &DDRA,
		.port_addr = (u8 *) &PORTA,
		.pin_addr = (u8 *) &PINA,
		.pin = 4,
		.direction = GPIO_INPUT,
		.pull_up = 1
	},
	{ // 8: input 10
		.direction_addr = (u8 *) &DDRA,
		.port_addr = (u8 *) &PORTA,
		.pin_addr = (u8 *) &PINA,
		.pin = 3,
		.direction = GPIO_INPUT,
		.pull_up = 1
	},

	/* List outputs */
	{ // 9: output 7
		.direction_addr = (u8 *) &DDRC,
		.port_addr = (u8 *) &PORTC,
		.pin = 2,
		.direction = GPIO_OUTPUT,
		.output_state = 0
	},
	{ // 10: output 6
		.direction_addr = (u8 *) &DDRC,
		.port_addr = (u8 *) &PORTC,
		.pin = 3,
		.direction = GPIO_OUTPUT,
		.output_state = 0
	},
	{ // 11: output 5
		.direction_addr = (u8 *) &DDRC,
		.port_addr = (u8 *) &PORTC,
		.pin = 4,
		.direction = GPIO_OUTPUT,
		.output_state = 0
	},
	{ // 12: output 4
		.direction_addr = (u8 *) &DDRC,
		.port_addr = (u8 *) &PORTC,
		.pin = 5,
		.direction = GPIO_OUTPUT,
		.output_state = 0
	},
	{ // 13: output 3
		.direction_addr = (u8 *) &DDRC,
		.port_addr = (u8 *) &PORTC,
		.pin = 6,
		.direction = GPIO_OUTPUT,
		.output_state = 0
	},
	{ // 14: output 2
		.direction_addr = (u8 *) &DDRC,
		.port_addr = (u8 *) &PORTC,
		.pin = 7,
		.direction = GPIO_OUTPUT,
		.output_state = 0
	},
	{ // 15: output 1
		.direction_addr = (u8 *) &DDRG,
		.port_addr = (u8 *) &PORTG,
		.pin = 2,
		.direction = GPIO_OUTPUT,
		.output_state = 0
	},

	/* List leds */
	{ // 16: led_rx
		.direction_addr = (u8 *) &DDRE,
		.port_addr = (u8 *) &PORTE,
		.pin = 2,
		.direction = GPIO_OUTPUT,
		.output_state = 0
	},
	{ // 17: led_tx
		.direction_addr = (u8 *) &DDRE,
		.port_addr = (u8 *) &PORTE,
		.pin = 3,
		.direction = GPIO_OUTPUT,
		.output_state = 0
	},
	{
		.direction_addr = NULL,
		.port_addr = NULL,
	},
};

struct gpio_input line_inputs[] = {
		{
			.num = 1,
			.gpio = gpio_list + 0,
		},
};

struct led led_green = {
	.gpio = gpio_list + 16
};

struct led led_red = {
	.gpio = gpio_list + 17
};


struct uart uart_debug = {
	.chip_id = 0,
	.baud_rate = 9600,
	.fdev_type = 1
};


struct uart uart_pc = {
	.chip_id = 1,
	.baud_rate = 9600,
	.fdev_type = 0
};


/**
 * Init board hardware
 */
static void board_init(void)
{
	u16 power_on_cnt;

	sys_timer_init();
	usart_init(&uart_debug);
	usart_init(&uart_pc);

	gpio_register_list(gpio_list);
	led_register(&led_red);
	led_register(&led_green);
//	led_set_blink(&led_red, 300, 300, 0);

	eeprom_init_fs();

//	wdt_enable(WDTO_2S);

	sei();
}


int main(void)
{
	board_init();
	printf("Init - ok\r\n");

	for(;;)
		idle();
}

