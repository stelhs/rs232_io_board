#include <avr/interrupt.h>
#include "types.h"
#include "gpio.h"
#include "leds.h"

static void led_timer_handler(void *arg)
{
	struct led *led = (struct led *)arg;

	if(!led->blink_timer)
		return;

	if(led->blink_timer > 1) {
		led->blink_timer--;
	}

	if(led->blink_timer > 1 || led->interval1 == 0) {
		return;
	}

	if (led->blink_counter == 1) {
		gpio_set_value(led->gpio, OFF);
		led->blink_timer = 0;
		led->blink_counter = 0;
		led->state = 0;
		return;
	}

	if(led->state) {
		gpio_set_value(led->gpio, OFF);
		led->blink_timer = led->interval2;
		led->state = 0;
	} else  {
		gpio_set_value(led->gpio, ON);
		led->blink_timer = led->interval1;
		if (led->blink_counter > 1)
			led->blink_counter--;
		led->state = 1;
	}
}


/**
 * Register new led
 * @param led - struct allocated not in stack with one necessary parameter: gpio
 */
void led_register(struct led *led)
{
	gpio_set_value(led->gpio, OFF);
	led->blink_timer = 0;
	led->interval1 = 0;
	led->interval2 = 0;

	led->timer.devisor = 10;
	led->timer.priv = led;
	led->timer.handler = led_timer_handler;
	sys_timer_add_handler(&led->timer);
}

/**
 * enable led light
 */
void led_on(struct led *led)
{
	gpio_set_value(led->gpio, ON);
	led->interval1 = 0;
	led->interval2 = 0;
	led->blink_timer = 0;
}

/**
 * disable led light
 */
void led_off(struct led *led)
{
	gpio_set_value(led->gpio, OFF);
	led->interval1 = 0;
	led->interval2 = 0;
	led->blink_timer = 0;
}

/**
 * set led blinking mode.
 * @param led - led descriptor
 * @param interval1 - light interval
 * @param interval2 - darkness interval
 * @param count - blink count. 0 for not limit
 */
void led_set_blink(struct led *led, t_counter interval1,
			t_counter interval2, int count)
{
	if(interval2 == 0)
		interval2 = interval1;

	gpio_set_value(led->gpio, ON);
	cli();
	led->interval1 = interval1 / 10 + 1;
	led->interval2 = interval2 / 10 + 1;
	led->blink_timer = led->interval1;
	led->blink_counter = count ? (count + 1) : 0;
	sei();
}


