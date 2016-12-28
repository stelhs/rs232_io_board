#include "gpio_keys.h"


#define GPIO_KEY_CLICK_INTERVAL 50
#define GPIO_KEY_HOLD_INTERVAL 100


static void gpio_keys_timer_handler(void *arg)
{
	struct gpio_key *key = (struct gpio_key *)arg;

	if (key->input.stable_state != key->prev_state) {
		key->prev_state = key->input.stable_state;

		/* check new button state */
		if (key->input.stable_state == 0) {
			/* Press */
			key->press_down_action = 1;
			key->click_timer = GPIO_KEY_CLICK_INTERVAL;
			key->hold_timer = GPIO_KEY_HOLD_INTERVAL;
			key->hold_counter = 0;
		} else {
			/* Unpress */
			key->press_up_action = 1;
			key->hold_timer = 0;
			key->hold_counter = 0;

			/* if on_click timeout not elapsed
			 *  this mean on_click happen */
			if (key->click_timer > 1) {
				key->click_action = 1;
				key->click_timer = 0;
			}
		}
	}

	if (key->hold_timer == 1) {
		key->hold_action = 1;
		key->hold_timer = GPIO_KEY_HOLD_INTERVAL;
		key->hold_counter++;
	}

	/* decrement timers */
	if (key->click_timer > 1)
		key->click_timer--;

	if (key->hold_timer > 1)
		key->hold_timer--;

}



static void gpio_keys_tsk(void *arg)
{
	struct gpio_key *key = (struct gpio_key *)arg;

	if (key->on_press_down && key->press_down_action) {
		key->on_press_down(key->priv);
		key->press_down_action = 0;
	}

	if (key->on_press_up && key->press_up_action) {
		key->on_press_up(key->priv);
		key->press_up_action = 0;
	}

	if (key->on_click && key->click_action) {
		key->on_click(key->priv);
		key->click_action = 0;
	}

	if (key->on_hold && key->hold_action) {
		key->on_hold(key->priv, key->hold_counter);
		key->hold_action = 0;
	}
}



void gpio_keys_register_key(struct gpio_key *key)
{
	gpio_debouncer_register_input(&key->input);
	key->prev_state = key->input.stable_state = 1;
	key->timer.devisor = 10;
	key->timer.priv = key;
	key->timer.handler = gpio_keys_timer_handler;
	sys_timer_add_handler(&key->timer);

	key->wrk.priv = key;
	key->wrk.handler = gpio_keys_tsk;
	sys_idle_add_handler(&key->wrk);
}






