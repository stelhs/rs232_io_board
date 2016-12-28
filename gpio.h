#ifndef GPIO_H
#define GPIO_H

#include "types.h"

enum gpio_direction {
	GPIO_INPUT,
	GPIO_OUTPUT
};

struct gpio {
	u8 *direction_addr;
	u8 *port_addr;
	u8 *pin_addr;
	u8 pin;
	enum gpio_direction direction;
	union {
		u8 output_state: 1; // current output state
		u8 pull_up: 1; // enable pull up resistor
	};
};

void gpio_register_list(struct gpio *gpio_list);
void gpio_set_direction(struct gpio *gpio, enum gpio_direction dir);
void gpio_set_value(struct gpio *gpio, u8 mode);

/**
 * Get GPIO pin input state
 * @param gpio - GPIO pin desriptor
 */
static inline int gpio_get_state(struct gpio *gpio)
{
	return ((gpio->pin_addr[0] & (1 << gpio->pin)) > 0);
}

#endif
