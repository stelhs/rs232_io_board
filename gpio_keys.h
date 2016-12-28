#ifndef GPIO_KEYS_H
#define GPIO_KEYS_H

#include "types.h"
#include "gpio.h"
#include "gpio_debouncer.h"

struct gpio_key
{
	struct gpio_input input;
	void (*on_press_down)(void *);
	void (*on_press_up)(void *);
	void (*on_click)(void *);
	void (*on_hold)(void *, t_counter hold_counter);
	void *priv;

// Private:
	struct sys_timer timer;
	struct sys_work wrk;
	u8 prev_state :1;
	u8 press_down_action :1;
	u8 press_up_action :1;
	u8 click_action :1;
	u8 hold_action :1;

	t_counter click_timer;
	t_counter hold_timer;
	t_counter hold_counter;
};

void gpio_keys_register_key(struct gpio_key *key);

#endif // GPIO_KEYS_H
