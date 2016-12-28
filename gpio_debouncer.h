#ifndef GPIO_DEBOUNCER_H
#define GPIO_DEBOUNCER_H

#include "types.h"
#include "gpio.h"
#include "sys_timer.h"
#include "idle.h"
#include "list.h"

#define GPIO_DEBOUNCE_INTERVAL 50

struct gpio_input
{
	u8 num;
	struct gpio *gpio;
	void (*on_change)(void *, u8);
	u8 stable_state :1;
	void *priv;

// Private:
	struct le le;
	u8 changed :1;
	u8 prev_state :1;
	t_counter debounce_counter;
	struct sys_timer timer;
	struct sys_work wrk;
};

void gpio_debouncer_register_input(struct gpio_input *input);

#endif
