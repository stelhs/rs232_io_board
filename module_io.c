/*
 * module_io.c
 *
 *  Created on: 29.12.2016
 *      Author: Michail Kurochkin
 */

#include <stdio.h>
#include <string.h>
#include <avr/wdt.h>
#include "board.h"
#include "types.h"
#include "gpio_debouncer.h"
#include "nmea0183.h"
#include "module_io.h"


static int send_input_line_state(struct mio *io, int input_num, int imput_state)
{
	struct nmea_msg msg;

	nmea_msg_reset(&msg);
	msg.ti = NMEA_TI_IO;
	msg.si = NMEA_SI_AIP;
	sprintf(msg.argv[1], "%d", 0);
	sprintf(msg.argv[2], "%d", input_num);
	sprintf(msg.argv[3], "%d", imput_state);
	msg.argc = 3;
	return nmea_send_msg(io->nmea_if, &msg);
}


static int
send_responce_line_state(struct mio *io, int sender_id, int input_num, int imput_state)
{
	struct nmea_msg msg;

	nmea_msg_reset(&msg);
	msg.ti = NMEA_TI_IO;
	msg.si = NMEA_SI_SIP;
	sprintf(msg.argv[1], "%d", sender_id);
	sprintf(msg.argv[2], "%d", input_num);
	sprintf(msg.argv[3], "%d", imput_state);
	msg.argc = 3;
	return nmea_send_msg(io->nmea_if, &msg);
}


static int
send_output_line_state(struct mio *io, int sender_id, int output_num, int output_state)
{
	struct nmea_msg msg;

	nmea_msg_reset(&msg);
	msg.ti = NMEA_TI_IO;
	msg.si = NMEA_SI_SOP;
	sprintf(msg.argv[1], "%d", sender_id);
	sprintf(msg.argv[2], "%d", output_num);
	sprintf(msg.argv[3], "%d", output_state);
	msg.argc = 3;
	return nmea_send_msg(io->nmea_if, &msg);
}

void usio_input_on_change(struct gpio_input *gi, u8 new_state)
{
	struct mio *io = (struct mio *)gi->priv;
	int input_num = gi->num;
	new_state = !new_state;

	led_set_blink(&led_red, 200, 300, 1);
	switch (io->io_mode) {
	case IO_MODE_MAIN:
		printf("input %d changed to %d\r\n", input_num, new_state);
		io->new_input_states[input_num - 1] = new_state;
		io->inputs_changed = 1;
		break;

	case IO_MODE_DEBUG:
		return;
	}
}

static void io_work(struct mio *io)
{
	int rc, i;
	wdt_reset();

	io->inputs_changed = 0;
	for (i = 0; i < IO_INPUTS_NUM; i++) {
		if (io->new_input_states[i] == io->input_states[i])
			continue;

		rc = send_input_line_state(io, i + 1,
					io->new_input_states[i]);
		if (rc)
			continue;

		io->input_states[i] = io->new_input_states[i];
	}
}


int usio_get_input_state(struct mio *io, int input_num)
{
	return io->input_states[input_num - 1];
}

void usio_relay_set_state(struct mio *io, int relay_num, int new_state)
{
	if (relay_num < 1 || relay_num > IO_OUTPUS_NUM)
		return;

	if (new_state < 0 || new_state > 1)
		return;

	led_set_blink(&led_green, 200, 300, 1);

	io->relay_states[relay_num - 1] = new_state;
	relay_set_state(relay_num, new_state);
}


static void nmea_rx_msg(struct nmea_msg *msg, struct mio *io)
{
	int output_num, output_state, input_num, sender_id;

	if (msg->ti != NMEA_TI_PC)
		return;

	switch (msg->si) {
	case NMEA_SI_RWS: // Set relay state
		if (msg->argc < 4)
			break;

		sscanf(msg->argv[1], "%d", &sender_id);
		sscanf(msg->argv[2], "%d", &output_num);
		sscanf(msg->argv[3], "%d", &output_state);
		switch (io->io_mode) {
		case IO_MODE_MAIN:
			usio_relay_set_state(io, output_num, output_state);
			break;

		case IO_MODE_DEBUG:
			io->relay_states[output_num - 1] = output_state;
			break;
		}
		send_output_line_state(io, sender_id, output_num, output_state);
		break;

	case NMEA_SI_RRS: // Get curent relay state
		if (msg->argc < 3)
			break;

		sscanf(msg->argv[1], "%d", &sender_id);
		sscanf(msg->argv[2], "%d", &output_num);
		send_output_line_state(io, sender_id, output_num,
				io->relay_states[output_num - 1]);
		break;

	case NMEA_SI_RIP:
		if (msg->argc < 3)
			break;

		sscanf(msg->argv[1], "%d", &sender_id);
		sscanf(msg->argv[2], "%d", &input_num);
		send_responce_line_state(io, sender_id, input_num,
				usio_get_input_state(io, input_num));
		break;

	default:
		return;
	}
}



int usio_init(struct mio *io, struct nmea_if *nmea_if)
{
	int i;

	memset(io, 0, sizeof(*io));
	io->nmea_if = nmea_if;

	for (i = 1; i <= IO_OUTPUS_NUM; i++)
		usio_relay_set_state(io, i, 0);

	io->ns.rx_callback = (void (*)(struct nmea_msg *, void *))nmea_rx_msg;
	io->ns.priv = io;
	nmea_add_subscriber(nmea_if, &io->ns);

	io->wrk.handler = (void (*)(void *))io_work;
	io->wrk.priv = io;
	sys_idle_add_handler(&io->wrk);

	io->io_mode = IO_MODE_MAIN;
	return 0;
}
