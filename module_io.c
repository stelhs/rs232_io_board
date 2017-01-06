/*
 * module_io.c
 *
 *  Created on: 29.12.2016
 *      Author: Michail Kurochkin
 */

#include <stdio.h>
#include <string.h>
#include "board.h"
#include "types.h"
#include "gpio_debouncer.h"
#include "nmea0183.h"
#include "module_io.h"
#include "module_io_wdt.h"


static int send_input_line_state(struct io_module *io, int input_num, int imput_state)
{
	struct nmea_msg msg;

	nmea_msg_reset(&msg);
	msg.ti = NMEA_TI_IO;
	msg.si = NMEA_SI_AIP;
	sprintf(msg.argv[1], "%d", input_num);
	sprintf(msg.argv[2], "%d", imput_state);
	msg.argc = 2;
	return nmea_send_msg(&io->nmea_if, &msg);
}


static int send_output_line_state(struct io_module *io, int output_num, int output_state)
{
	struct nmea_msg msg;

	nmea_msg_reset(&msg);
	msg.ti = NMEA_TI_IO;
	msg.si = NMEA_SI_SOP;
	sprintf(msg.argv[1], "%d", output_num);
	sprintf(msg.argv[2], "%d", output_state);
	msg.argc = 2;
	return nmea_send_msg(&io->nmea_if, &msg);
}


void io_input_on_change(struct gpio_input *gi, u8 new_state)
{
	struct io_module *io = (struct io_module *)gi->priv;
	int input_num = gi->num - 1;
	new_state = !new_state;

	led_set_blink(&led_red, 200, 300, 1);
	switch (io->io_mode) {
	case IO_MODE_MAIN:
		printf("input %d changed to %d\r\n", input_num, new_state);
		io->new_input_states[input_num] = new_state;
		io->inputs_changed = 1;
		break;

	case IO_MODE_DEBUG:
		return;
	}
}

static void io_work(struct io_module *io)
{
	int rc, i;
	if (!io->inputs_changed)
		return;

	io->inputs_changed = 0;
	for (i = 0; i < IO_INPUTS_NUM; i++) {
		if (io->new_input_states[i] == io->input_states[i])
			continue;

		printf("send_input_line_state %d, %d\r\n", i, io->new_input_states[i]);
		rc = send_input_line_state(io, i + 1,
					io->new_input_states[i]);
		if (rc)
			continue;

		io->input_states[i] = io->new_input_states[i];
	}
}


int io_get_input_state(struct io_module *io, int input_num)
{
	return io->input_states[input_num];
}

void io_output_set_state(struct io_module *io, int output_num, int new_state)
{
	if (output_num < 1 || output_num > IO_OUTPUS_NUM)
		return;

	if (new_state < 0 || new_state > 1)
		return;

	led_set_blink(&led_green, 200, 300, 1);
	printf("output %d changed to %d\r\n", output_num, new_state);

	io->output_states[output_num] = new_state;
	relay_set_state(output_num, new_state);
	send_output_line_state(io, output_num, new_state);
}


void nmea_rx_msg(struct nmea_msg *msg, struct io_module *io)
{
	int output_num, output_state, input_num, state;

	if (msg->ti != NMEA_TI_PC)
		return;

	switch (msg->si) {
	case NMEA_SI_RWS: // Set relay state
		if (msg->argc < 3)
			break;

		sscanf(msg->argv[1], "%d", &output_num);
		sscanf(msg->argv[2], "%d", &output_state);
		switch (io->io_mode) {
		case IO_MODE_MAIN:
			io_output_set_state(io, output_num, output_state);
			break;

		case IO_MODE_DEBUG:
			io->output_states[output_num] = output_state;
			break;
		}
		break;

	case NMEA_SI_RRS: // Get curent relay state
		if (msg->argc < 2)
			break;

		sscanf(msg->argv[1], "%d", &output_num);
		send_output_line_state(io, output_num,
				io->output_states[output_num]);
		break;

	case NMEA_SI_RIP:
		if (msg->argc < 2)
			break;

		sscanf(msg->argv[1], "%d", &input_num);
		send_input_line_state(io, input_num,
				io_get_input_state(io, input_num));
		break;

	case NMEA_SI_WDS:
		if (msg->argc < 2)
			break;

		if (!io->wdt)
			break;

		sscanf(msg->argv[1], "%d", &state);

		if (state) {
			printf("WDT ENABLE\r\n");
			io_wdt_enable(io->wdt);
		} else {
			printf("WDT DISABLE\r\n");
			io_wdt_disable(io->wdt);
		}
		break;

	case NMEA_SI_WRS:
		printf("WDT RST\r\n");
		if (io->wdt)
			io_wdt_reset(io->wdt);
		break;

	default:
		return;
	}
}



int module_io_init(struct io_module *io, int uart_id)
{
	int rc, i;

	memset(io, 0, sizeof(*io));

	for (i = 1; i <= IO_OUTPUS_NUM; i++)
		io_output_set_state(io, i, 0);

	rc = nmea_register(&io->nmea_if, uart_id, 9600,
		(void (*)(struct nmea_msg *, void *)) nmea_rx_msg);
	if (rc)
		return rc;

	io->wrk.handler = (void (*)(void *))io_work;
	io->wrk.priv = io;
	sys_idle_add_handler(&io->wrk);

	io->io_mode = IO_MODE_MAIN;
	io->nmea_if.priv = io;
	return 0;
}
