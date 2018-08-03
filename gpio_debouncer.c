#include "types.h"
#include "gpio_debouncer.h"
#include <avr/interrupt.h>


static void gpio_debouncer_timer_handler(struct gpio_input *input)
{
	u8 curr_state;

	curr_state = gpio_get_state(input->gpio);
	if (input->prev_state != curr_state) {
		input->debounce_counter = GPIO_DEBOUNCE_INTERVAL;
		input->prev_state = curr_state;
	}

	if (input->debounce_counter > 1)
		input->debounce_counter --;

	if (input->debounce_counter == 1) {
		input->stable_state = curr_state;
		input->changed = 1;
		input->debounce_counter = 0;
	}
}


static void gpio_debouncer_tsk(void *arg)
{
	struct gpio_input *input = (struct gpio_input *)arg;
	cli();
	if (input->on_change && input->changed) {
		input->on_change(input, input->stable_state);
		input->changed = 0;
	}
	sei();
}


void gpio_debouncer_register_input(struct gpio_input *input)
{
	input->changed = 0;
	input->debounce_counter = 0;
	input->prev_state = 0;
	input->stable_state = 0;

	input->timer.devisor = 1;
	input->timer.priv = input;
	input->timer.handler = (void (*)(void *))gpio_debouncer_timer_handler;
	sys_timer_add_handler(&input->timer);

	input->wrk.priv = input;
	input->wrk.handler = gpio_debouncer_tsk;
	sys_idle_add_handler(&input->wrk);
}

void gpio_debouncer_register_list_inputs(struct gpio_input *list_inputs)
{
	struct gpio_input *input;
	for (input = list_inputs; input->gpio; input++) {
		gpio_debouncer_register_input(input);
		input->changed = 1;
	}
}
