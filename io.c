/*
 * io.c
 *
 *  Created on: 29.12.2016
 *      Author: Michail Kurochkin
 */

#include <stdio.h>
#include "board.h"
#include "types.h"
#include "gpio.h"
#include "io.h"

void relay_set_state(int relay_num, int state)
{
	struct gpio *gpio;
	switch (relay_num) {
	case 1: gpio = gpio_list + 15; break;
	case 2: gpio = gpio_list + 14; break;
	case 3: gpio = gpio_list + 13; break;
	case 4: gpio = gpio_list + 12; break;
	case 5: gpio = gpio_list + 11; break;
	case 6: gpio = gpio_list + 10; break;
	case 7: gpio = gpio_list + 9; break;
	default: return;
	}

	gpio_set_value(gpio, state);
}

void line_input_on_change(struct gpio_input *gi, u8 new_state)
{
	printk("changed input %d\r\n", gi->num);
}
