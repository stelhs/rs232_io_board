/*
 * io.h
 *
 *  Created on: 29.12.2016
 *      Author: Michail Kurochkin
 */

#ifndef MODULE_IO_H_
#define MODULE_IO_H_

#include "gpio_debouncer.h"
#include "leds.h"
#include "types.h"
#include "nmea0183.h"
#include "module_io_wdt.h"

#define IO_INPUTS_NUM 10 /* Input lines number */
#define IO_OUTPUS_NUM 7 /* Output lines number */

struct io_module {
	struct nmea_if nmea_if;
	struct sys_work wrk;

	/* IO module modes */
	enum io_modes {
		IO_MODE_MAIN,
		IO_MODE_DEBUG,
	} io_mode;

	u8 input_states[IO_INPUTS_NUM];
	u8 new_input_states[IO_INPUTS_NUM];
	u8 inputs_changed :1;

	u8 output_states[IO_OUTPUS_NUM];

	struct io_module_wdt *wdt;
};

void io_input_on_change(struct gpio_input *gi, u8 new_state);
void io_output_set_state(struct io_module *io, int output_num, int new_state);
int io_get_input_state(struct io_module *io, int input_num);
int module_io_init(struct io_module *io, int uart_id);

#endif /* MODULE_IO_H_ */



