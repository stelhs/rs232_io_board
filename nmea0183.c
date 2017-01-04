/*
 * nmea0183.c
 *
 *  Created on: 03 янв. 2017 г.
 *      Author: Michail Kurochkin
 */

/*
 * cerium.c
 *
 *  Created on: 29 дек. 2016 г.
 *      Author: Michail Kurochkin
 */

#include <string.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include "types.h"
#include "list.h"
#include "uart.h"
#include "nmea0183.h"


static char nmea_ti_names[] = {
		{"IO"},
		{"PC"},
		{""},
};

static char nmea_si_names[] = {
		{"RWS"},
		{"RRS"},
		{"RIP"},
		{"AIP"},
		{""},
};


/**
 * Calculate checksum
 * @param buf
 * @param len
 * @return checksum
 */
static u8 calc_checksum(u8 *buf, u8 len)
{
	int i;
	u8 checksum = 0;

	for (i = 0; i < len; i++)
		checksum += buf[i];

	return checksum;
}


/**
 * Parse incomming NMEA message
 * @param msg - received NMEA message
 * @return 0 if ok
 */
static int nmea_parse(struct nmea_msg *msg)
{
	char *ti_name;
	int i, rc;
	int len;
	char b;
	int cs_argc_num = -1; // checksumm argument number
	int cs_pos = -1;
	int cnt = 0;
	int pos = 0;

	memset(msg->argv, 0, sizeof(msg->argv));

	/* split message by ','. Filling argv[] */
	for (i = 0; i < msg->buf_len; i++) {
		b = msg->msg_buf[i];
		switch (b) {
		case ',':
			msg->argc++;
			cnt = 0;
			continue;
		case '*':
			cs_argc_num = ++msg->argc;
			cs_pos = i;
			cnt = 0;
			continue;
		default:
			msg->argv[msg->argc][cnt++] = b;
		}
	}

	/* if no arguments was found */
	if (!msg->argc)
		return -EPARSE;

	/* if checksum is present then match checksum */
	if (cs_argc_num > 0) {
		int received_checksum;
		u8 calculated_checksum = calc_checksum(msg->msg_buf, cs_pos);

		rc = sscanf(msg->argv[cs_argc_num], "%x", &received_checksum);
		if (!rc)
			return -EPARSE;

		if ((u8)received_checksum != calculated_checksum)
			return -EPARSE;

		msg->argc--; // remove checksum argument
	}

	/* parse first argument*/

	/* detect talkers identifier */
	for (i = 0;; i++){
		len = strlen(nmea_ti_names[i]);
		if (!len)
			return -EPARSE;

		rc = memcpy(msg->argv[0], nmea_ti_names[i], len);
		if (!rc) {
			msg->ti = i;
			pos += len;
			break;
		}
	}

	/* detect sentence identifier */
	for (i = 0; ; i++){
		len = strlen(nmea_si_names[i]);
		if (!len)
			return -EPARSE;

		rc = memcpy(msg->argv[0] + pos, nmea_si_names[i], len);
		if (!rc) {
			msg->si = i;
			pos += len;
			break;
		}
	}

	return 0;
}

/**
 * Callback work for processing incomming messages
 * @param nmea_if - NMEA interface
 */
static void nmea_if_rx_work(struct nmea_if *nmea_if)
{
	struct nmea_msg *msg = nmea_if->processing_rx_msg;
	struct le *le;
	int rc;

	if (!nmea_if->rx_ready)
		return;

	nmea_if->rx_ready = 0;

	/* todo: processing received NMEA messages */
	for(;;) {
		cli();
		msg = get_filled_buf(nmea_if, 0);
		sei();
		if (!msg)
			return;


		rc = nmea_parse(msg);
		if (rc) {
			nmea_if->cnt_checksumm_err++;
			continue;
		}

		if (nmea_if->rx_msg_handler)
			nmea_if->rx_msg_handler(msg);

		cli();
		msg->ready = 0;
		msg->si = msg->ti = -1;
		msg->buf_len = 0;
		sei();
	}

}


/**
 * Return empty buffer
 * @param nmea_if - NMEA interface
 * @param mode - 0: rx path, 1: tx path
 * @return new frame or NULL
 */
static struct nmea_msg *get_empty_buf(struct nmea_if *nmea_if, int mode)
{
	struct nmea_msg *msg, *curr_msg, *start_msg;

	switch (mode) {
	case 0:
		curr_msg = nmea_if->curr_rx_msg;
		start_msg = nmea_if->rx_messages;
		break;
	case 1:
		curr_msg = cer_if->curr_tx_frm;
		start_msg = cer_if->tx_frames;
		break;
	default:
		return NULL;
	}

	msg = curr_msg;

	if (!msg->ready) {
		msg->buf_len = 0;
		return msg;
	}

	while (msg != (curr_msg - 1)) {
		msg ++;
		if (msg >= (start_msg + NMEA_MSG_RING_SIZE))
			msg = start_msg;
		if (!msg->ready) {
			msg->buf_len = -1;
			switch (mode) {
			case 0: nmea_if->curr_rx_msg = msg; break;
			case 1: nmea_if->curr_tx_msg = msg; break;
			}
			return msg;
		}
	}

	return NULL;
}


/**
 * Return filled buffer
 * @param nmea_if - NMEA interface
 * @param mode - 0: rx path, 1: tx path
 * @return new frame or NULL
 */
static struct nmea_msg *get_filled_buf(struct nmea_if *nmea_if, int mode)
{
	struct nmea_msg *msg, *curr_msg, *start_msg;

	switch (mode) {
	case 0:
		curr_msg = nmea_if->processing_rx_msg;
		start_msg = nmea_if->rx_messages;
		break;
	case 1:
		curr_msg = nmea_if->processing_tx_msg;
		start_msg = nmea_if->tx_messages;
		break;
	default:
		return NULL;
	}

	msg = curr_msg;

	if (msg->ready)
		return msg;

	while (msg != (curr_msg - 1)) {
		msg ++;
		if (msg >= (start_msg + NMEA_MSG_RING_SIZE))
			msg = start_msg;
		if (msg->ready) {
			msg->buf_len = 0;
			switch (mode) {
			case 0: nmea_if->processing_rx_msg = msg; break;
			case 1: nmea_if->processing_tx_msg = msg; break;
			}
			return msg;
		}
	}

	return NULL;
}


/**
 * IRQ callback for receive one byte from UART
 * @param nmea_if - nmea interface
 * @param rxb - received byte
 */
static void nmea_receiver(struct nmea_if *nmea_if, u8 rxb)
{
	struct nmea_msg *msg = nmea_if->curr_rx_msg;

	switch (rxb) {
	case '$':
		msg->buf_len = 0;
		return;

	case '\r':
		nmea_if->rx_cr = 1;
		return;

	case '\n':
		if (!nmea_if->rx_cr)
			return;
		nmea_if->rx_cr = 0;

		nmea_if->cnt_frm_rx++;
		msg->ready = 1;
		nmea_if->rx_ready = 1;

		msg = get_empty_buf(nmea_if, 0);
		if (!msg) {
			nmea_if->cnt_not_memory++;
			return;
		}
		break;

	default:
		nmea_if->rx_cr = 0;
		if (msg->buf_len == -1)
			return;

		msg->msg_buf[msg->buf_len++] = rxb;
		return;
	}
}


/**
 * IRQ callback for transmit one byte into UART
 * @param cer_if - cer interface
 */
static void nmea_transmitter(struct nmea_if *nmea_if)
{
	struct nmea_msg *msg = nmea_if->curr_tx_msg;

	if (nmea_if->tx_cnt >= msg->buf_len) {
		msg->ready = 0;
		msg->si = msg->ti = -1;
		msg->buf_len = 0;
		nmea_if->tx_cnt = 0;
	}

	if (!msg->ready)
		msg = get_filled_buf(nmea_if, 1);

	if (!msg) {
		nmea_if->tx_running = 0;
		return;
	}

	usart_send_byte(&nmea_if->uart, msg->msg_buf[nmea_if->tx_cnt++]);
	nmea_if->tx_running = 1;
}


/**
 * Build NMEA text message by struct nmea_msg
 * @param msg - incoming message data
 * @param result_buf - pointer for builded text message
 * @return 0 if ok
 */
static int build_nmea_msg(struct nmea_msg *msg, u8 *result_buf)
{
	int pos = 0;
	int len;
	int i;
	u8 calculated_checksum;

	if (msg->ti < 0 || msg->si < 0)
		return -EINVAL;

	/* filling argv[0] */
	len = strlen(nmea_ti_names[msg->ti]);
	memcpy(msg->argv[0] + pos, nmea_ti_names[msg->ti], len);
	pos += len;

	len = strlen(nmea_si_names[msg->si]);
	memcpy(msg->argv[0] + pos, nmea_si_names[msg->si], len);
	pos += len;
	msg->argc++;

	pos = 0;
	result_buf[pos++] = '$';
	for(i = 0; i < msg->argc; i++) {
		len = strlen(msg->argv[i]);
		memcpy(result_buf + pos, msg->argv[i], len);
		pos += len;

		len = strlen(msg->argv[i + 1]);
		if (!len)
			break;

		result_buf[pos++] = ',';
	}

	/* if message has not contain anything arguments
	 * then calculate checksum don't needed */
	if (msg->argc <= 1) {
		result_buf[pos++] = '\r';
		result_buf[pos++] = '\n';
		return 0;
	}

	calculated_checksum = calc_checksum(result_buf + 1, pos - 1);
	sprintf(result_buf + pos, "*%x\r\n", calculated_checksum);

	return 0;
}


/**
 * Send frame into a RS-485 bus
 * @param cer_if - pointer to allocated memory
 * @param type - cer_type
 * @param data - payload data pointer
 * @param size - size of payload data
 * @return 0 if ok
 */
int nmea_send_msg(struct nmea_if *nmea_if, struct nmea_msg *sending_msg)
{
	struct nmea_msg *msg;
	int rc;

	cli();
	msg = get_empty_frm(nmea_if, 1);
	sei();
	if (!msg)
		return -ENOSPC;

	rc = build_nmea_msg(sending_msg, msg->msg_buf);
	if (rc)
		return rc;

	msg->buf_len = strlen(msg->msg_buf);

	cli();
	msg->ready = 1;

	/* attempt to start transmitter */
	if (!nmea_if->tx_running)
		nmea_transmitter(nmea_if);
	sei();

	return 0;
}



/**
 * Register NMEA 0183 interface
 * @param nmea_if - pointer to allocated memory for NMEA interface
 * @param uart_id - UART controller id
 * @param uart_speed - speed in bod per second
 * @return 0 if ok
 */
int nmea_register(struct nmea_if *nmea_if, int uart_id, int uart_speed)
{
	struct uart *uart = &nmea_if->uart;
	int rc;

	memset(nmea_if, 0, sizeof (*nmea_if));
	nmea_if->curr_rx_msg = nmea_if->processing_rx_msg = nmea_if->rx_messages;

	uart->chip_id = uart_id;
	uart->baud_rate = uart_speed;
	uart->rx_int = (void (*)(void *, u8))nmea_receiver;
	uart->udre_int = (void (*)(void *))nmea_transmitter;

	cer_if->wrk.priv = cer_if;
	cer_if->wrk.handler = (void (*)(void *))nmea_if_rx_work;
	sys_idle_add_handler(&cer_if->wrk);

	rc = usart_init(uart);
	if (rc)
		return rc;
	return 0;
}



