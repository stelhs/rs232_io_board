/*
 * io.h
 *
 *  Created on: 29.12.2016
 *      Author: Michail Kurochkin
 */

#ifndef IO_H_
#define IO_H_

#include "gpio_debouncer.h"
#include "leds.h"
#include "types.h"

void relay_set_state(int relay_num, int state);
void line_input_on_change(struct gpio_input *gi, u8 new_state);

#endif /* IO_H_ */



