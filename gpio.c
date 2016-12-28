#include <stdio.h>
#include "types.h"
#include "gpio.h"

/**
 * Set GPIO direction
 * @param gpio - GPIO pin desriptor
 * @param mode - 0 - input, 1 - output
 */
void gpio_set_direction(struct gpio *gpio, enum gpio_direction dir)
{
	if (dir)
		gpio->direction_addr[0] |= (1 << gpio->pin);
	else
		gpio->direction_addr[0] &= ~(1 << gpio->pin);
	gpio->direction = dir;
}

/**
 * Set GPIO output state
 * @param gpio - GPIO pin desriptor
 * @param mode - ON or OFF
 */
void gpio_set_value(struct gpio *gpio, u8 mode)
{
	if (mode)
		gpio->port_addr[0] |= (1 << gpio->pin);
	else
		gpio->port_addr[0] &= ~(1 << gpio->pin);

	gpio->output_state = mode;
}


/**
 * Init GPIO list
 * @param gpio_list
 */
void gpio_register_list(struct gpio *gpio_list)
{
	struct gpio *gpio;
	for (gpio = gpio_list; gpio->direction_addr != NULL; gpio++) {
		gpio_set_direction(gpio, gpio->direction);
		gpio_set_value(gpio, gpio->output_state);
	}
}
