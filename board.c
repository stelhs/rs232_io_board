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
#include "io.h"

struct gpio gpio_list[] = {
	/* List inputs */
	{ // 0: input 6
		.direction_addr = (u8 *) &DDRA,
		.port_addr = (u8 *) &PORTA,
		.pin_addr = (u8 *) &PINA,
		.pin = 0,
		.direction = GPIO_INPUT,
		.pull_up = 1
	},
	{ // 1: input 5
		.direction_addr = (u8 *) &DDRA,
		.port_addr = (u8 *) &PORTA,
		.pin_addr = (u8 *) &PINA,
		.pin = 1,
		.direction = GPIO_INPUT,
		.pull_up = 1
	},
	{ // 2: input 10
		.direction_addr = (u8 *) &DDRF,
		.port_addr = (u8 *) &PORTF,
		.pin_addr = (u8 *) &PINF,
		.pin = 0,
		.direction = GPIO_INPUT,
		.pull_up = 1
	},
	{ // 3: input 8
		.direction_addr = (u8 *) &DDRF,
		.port_addr = (u8 *) &PORTF,
		.pin_addr = (u8 *) &PINF,
		.pin = 2,
		.direction = GPIO_INPUT,
		.pull_up = 1
	},
	{ // 4: input 1
		.direction_addr = (u8 *) &DDRA,
		.port_addr = (u8 *) &PORTA,
		.pin_addr = (u8 *) &PINA,
		.pin = 5,
		.direction = GPIO_INPUT,
		.pull_up = 1
	},
	{ // 5: input 9
		.direction_addr = (u8 *) &DDRF,
		.port_addr = (u8 *) &PORTF,
		.pin_addr = (u8 *) &PINF,
		.pin = 1,
		.direction = GPIO_INPUT,
		.pull_up = 1
	},
	{ // 6: input 7
		.direction_addr = (u8 *) &DDRF,
		.port_addr = (u8 *) &PORTF,
		.pin_addr = (u8 *) &PINF,
		.pin = 3,
		.direction = GPIO_INPUT,
		.pull_up = 1
	},
	{ // 7: input 2
		.direction_addr = (u8 *) &DDRA,
		.port_addr = (u8 *) &PORTA,
		.pin_addr = (u8 *) &PINA,
		.pin = 4,
		.direction = GPIO_INPUT,
		.pull_up = 1
	},
	{ // 8: input 3
		.direction_addr = (u8 *) &DDRA,
		.port_addr = (u8 *) &PORTA,
		.pin_addr = (u8 *) &PINA,
		.pin = 3,
		.direction = GPIO_INPUT,
		.pull_up = 1
	},
	{ // 9: input 4
		.direction_addr = (u8 *) &DDRA,
		.port_addr = (u8 *) &PORTA,
		.pin_addr = (u8 *) &PINA,
		.pin = 2,
		.direction = GPIO_INPUT,
		.pull_up = 1
	},

	/* List outputs */
	{ // 10: output 7
		.direction_addr = (u8 *) &DDRC,
		.port_addr = (u8 *) &PORTC,
		.pin = 2,
		.direction = GPIO_OUTPUT,
		.output_state = 0
	},
	{ // 11: output 6
		.direction_addr = (u8 *) &DDRC,
		.port_addr = (u8 *) &PORTC,
		.pin = 3,
		.direction = GPIO_OUTPUT,
		.output_state = 0
	},
	{ // 12: output 5
		.direction_addr = (u8 *) &DDRC,
		.port_addr = (u8 *) &PORTC,
		.pin = 4,
		.direction = GPIO_OUTPUT,
		.output_state = 0
	},
	{ // 13: output 4
		.direction_addr = (u8 *) &DDRC,
		.port_addr = (u8 *) &PORTC,
		.pin = 5,
		.direction = GPIO_OUTPUT,
		.output_state = 0
	},
	{ // 14: output 3
		.direction_addr = (u8 *) &DDRC,
		.port_addr = (u8 *) &PORTC,
		.pin = 6,
		.direction = GPIO_OUTPUT,
		.output_state = 0
	},
	{ // 15: output 2
		.direction_addr = (u8 *) &DDRC,
		.port_addr = (u8 *) &PORTC,
		.pin = 7,
		.direction = GPIO_OUTPUT,
		.output_state = 0
	},
	{ // 16: output 1
		.direction_addr = (u8 *) &DDRG,
		.port_addr = (u8 *) &PORTG,
		.pin = 2,
		.direction = GPIO_OUTPUT,
		.output_state = 0
	},

	/* List leds */
	{ // 17: led_rx
		.direction_addr = (u8 *) &DDRE,
		.port_addr = (u8 *) &PORTE,
		.pin = 2,
		.direction = GPIO_OUTPUT,
		.output_state = 0
	},
	{ // 18: led_tx
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
		.gpio = gpio_list + 4,
		.on_change = line_input_on_change,
	},
	{
		.num = 2,
		.gpio = gpio_list + 7,
		.on_change = line_input_on_change,
	},
	{
		.num = 3,
		.gpio = gpio_list + 8,
		.on_change = line_input_on_change,
	},
	{
		.num = 4,
		.gpio = gpio_list + 9,
		.on_change = line_input_on_change,
	},
	{
		.num = 5,
		.gpio = gpio_list + 1,
		.on_change = line_input_on_change,
	},
	{
		.num = 6,
		.gpio = gpio_list + 0,
		.on_change = line_input_on_change,
	},
	{
		.num = 7,
		.gpio = gpio_list + 6,
		.on_change = line_input_on_change,
	},
	{
		.num = 8,
		.gpio = gpio_list + 3,
		.on_change = line_input_on_change,
	},
	{
		.num = 9,
		.gpio = gpio_list + 5,
		.on_change = line_input_on_change,
	},
	{
		.num = 10,
		.gpio = gpio_list + 2,
		.on_change = line_input_on_change,
	},
	{
		.gpio = NULL,
	},
};

struct led led_green = {
	.gpio = gpio_list + 17
};

struct led led_red = {
	.gpio = gpio_list + 18
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
	sys_timer_init();
	usart_init(&uart_debug);
	usart_init(&uart_pc);

	gpio_register_list(gpio_list);
	gpio_debouncer_register_list_inputs(line_inputs);
	led_register(&led_red);
	led_register(&led_green);
	led_set_blink(&led_red, 300, 300, 0);

	eeprom_init_fs();

//	wdt_enable(WDTO_2S);

	sei();
}

struct sys_work debug_wrk;
static void scan_keycode(void *arg)
{
	char key;
	key = usart_get_byte(&uart_debug);
	if (!key)
		return;

	printf("key=%c\r\n", key);
	switch (key) {
	case '1': relay_set_state(1, 1); break;
	case '!': relay_set_state(1, 0); break;

	case '2': relay_set_state(2, 1); break;
	case '@': relay_set_state(2, 0); break;

	case '3': relay_set_state(3, 1); break;
	case '#': relay_set_state(3, 0); break;

	case '4': relay_set_state(4, 1); break;
	case '$': relay_set_state(4, 0); break;

	case '5': relay_set_state(5, 1); break;
	case '%': relay_set_state(5, 0); break;

	case '6': relay_set_state(6, 1); break;
	case '^': relay_set_state(6, 0); break;
	}
}

int main(void)
{
	board_init();
	printf("Init - ok\r\n");

	debug_wrk.handler = scan_keycode;
	sys_idle_add_handler(&debug_wrk);

	for(;;)
		idle();
}

