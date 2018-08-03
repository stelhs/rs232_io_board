#ifndef GPIO_DEBOUNCER_H
#define GPIO_DEBOUNCER_H

#include "types.h"
#include "gpio.h"
#include "sys_timer.h"
#include "idle.h"
#include "list.h"

#define GPIO_DEBOUNCE_INTERVAL 20

struct gpio_input
{
	u8 num;
	struct gpio *gpio;
	void (*on_change)(struct gpio_input *, u8);
	u8 stable_state :1;
	void *priv;

// Private:
	struct le le;
	volatile u8 changed;
	volatile u8 prev_state;
	t_counter debounce_counter;
	struct sys_timer timer;
	struct sys_work wrk;
};

void gpio_debouncer_register_input(struct gpio_input *input);
void gpio_debouncer_register_list_inputs(struct gpio_input *list_inputs);

#endif
