/*
 * board.h
 *
 *  Created on: 29.12.2016
 *      Author: Michail Kurochkin
 */

#ifndef INIT_H_
#define INIT_H_

#include "gpio.h"
#include "leds.h"

#define CLEAR_WATCHDOG() { asm("wdr"); }

//extern struct gpio gpio_list[];
extern struct led led_red;
extern struct led led_green;
void relay_set_state(int relay_num, int state);

#endif /* INIT_H_ */
