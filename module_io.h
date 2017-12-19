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

#define IO_INPUTS_NUM 10 /* Input lines number */
#define IO_OUTPUS_NUM 7 /* Output lines number */

struct mio {
	struct sys_work wrk;

	/* IO module modes */
	enum io_modes {
		IO_MODE_MAIN,
		IO_MODE_DEBUG,
	} io_mode;

	u8 input_states[IO_INPUTS_NUM];
	u8 new_input_states[IO_INPUTS_NUM];
	u8 inputs_changed :1;

	u8 relay_states[IO_OUTPUS_NUM];

	struct nmea_if *nmea_if;
	struct nmea_subsriber ns;
};

void usio_input_on_change(struct gpio_input *gi, u8 new_state);
void usio_relay_set_state(struct mio *io, int output_num, int new_state);
int usio_get_input_state(struct mio *io, int input_num);
int usio_init(struct mio *io, struct nmea_if *nmea_if);

#endif /* MODULE_IO_H_ */



