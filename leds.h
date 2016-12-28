#ifndef LED_LIB_H
#define LED_LIB_H

#include "types.h"
#include "sys_timer.h"

struct led
{
	struct gpio *gpio;
// Private:
	u8 state : 1; // current state
	t_counter blink_timer; // timer counter
	t_counter interval1; // active interval
	t_counter interval2; // inactive interval
	int blink_counter; //
	struct sys_timer timer;
};

void led_register(struct led *led);

void led_on(struct led *led);
void led_off(struct led *led);
void led_set_blink(struct led *led, t_counter interval1,
			t_counter interval2, int count);

#endif
