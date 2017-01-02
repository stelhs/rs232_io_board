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
#include "cerium.h"


/**
 * CRC-16 CCITT calculator
 * @param data - buffer with data
 * @param len - lenght of data
 * @return crc16 value
 */
static u16 crc16(u8 *data, int len)
{
	u16 crc = 0xFFFF;
	u8 i;

	while(len--) {
		crc ^= *data++ << 8;
		for(i = 0; i < 8; i++)
			crc = crc & 0x8000 ? ( crc << 1 ) ^ 0x1021 : crc << 1;
	}

	return crc;
}

/**
 * Callback timer with resolution 1ms
 * @param cer_if - Cerium interface
 */
static void cer_if_timer(struct cer_if *cer_if)
{
	cer_if->rx_timeout_counter++;
	if (cer_if->rx_timeout_counter < 2)
		return;

	cer_if->rx_timeout = 1;
}


/**
 * Check for valid FCS
 * @param frm - pointer to beryllium frame
 * @return 1 if valid, 0 if not valid
 */
static int cer_frm_check_fcs(struct cer_frm *frm)
{
	u16 *crc_recv = (u16 *)(frm->data + CER_FRM_HDR_SIZE +
				frm->payload_size);
	u16 crc_calc = crc16(frm->data, CER_FRM_HDR_SIZE + frm->payload_size);
	return (crc_calc == *crc_recv);
}

/**
 * Return empty frame
 * @param cer_if - cer interface
 * @param mode - 0: rx path, 1: tx path
 * @return new frame or NULL
 */
static struct cer_frm *get_empty_frm(struct cer_if *cer_if, int mode)
{
	struct cer_frm *frm, *curr_frm, *start_frm;

	switch (mode) {
	case 0:
		curr_frm = cer_if->curr_rx_frm;
		start_frm = cer_if->rx_frames;
		break;
	case 1:
		curr_frm = cer_if->curr_tx_frm;
		start_frm = cer_if->tx_frames;
		break;
	default:
		return NULL;
	}

	frm = curr_frm;

	if (!frm->ready) {
		frm->payload_size = 0;
		return frm;
	}

	while (frm != (curr_frm - 1)) {
		frm ++;
		if (frm >= (start_frm + CER_RING_SIZE))
			frm = start_frm;
		if (!frm->ready) {
			frm->payload_size = 0;
			switch (mode) {
			case 0: cer_if->curr_rx_frm = frm; break;
			case 1: cer_if->curr_tx_frm = frm; break;
			}
			return frm;
		}
	}

	return NULL;
}


/**
 * Return filled frame
 * @param cer_if - cer interface
 * @param mode - 0: rx path, 1: tx path
 * @return new frame or NULL
 */
static struct cer_frm *get_filled_frm(struct cer_if *cer_if, int mode)
{
	struct cer_frm *frm, *curr_frm, *start_frm;

	switch (mode) {
	case 0:
		curr_frm = cer_if->processing_rx_frm;
		start_frm = cer_if->rx_frames;
		break;
	case 1:
		curr_frm = cer_if->processing_tx_frm;
		start_frm = cer_if->tx_frames;
		break;
	default:
		return NULL;
	}

	frm = curr_frm;

	if (frm->ready)
		return frm;

	while (frm != (curr_frm - 1)) {
		frm ++;
		if (frm >= (start_frm + CER_RING_SIZE))
			frm = start_frm;
		if (frm->ready) {
			frm->payload_size = 0;
			switch (mode) {
			case 0: cer_if->processing_rx_frm = frm; break;
			case 1: cer_if->processing_tx_frm = frm; break;
			}
			return frm;
		}
	}

	return NULL;
}


/**
 * Callback work for processing frames
 * @param cer_if - Cerium interface
 */
static void cer_if_rx_work(struct cer_if *cer_if)
{
	struct cer_frm *frm = cer_if->processing_rx_frm;
	struct le *le;
	u8 cer_type;
	u8 *cer_payload;

	if (!cer_if->rx_ready)
		return;

	cer_if->rx_ready = 0;

	/* todo: processing received frames */
	for(;;) {
		cli();
		frm = get_filled_frm(cer_if, 0);
		sei();
		if (!frm)
			return;

		if(!cer_frm_check_fcs(frm)) {
			cer_if->cnt_frm_rx_crc_err++;
			return;
		}

		/* Execute callbacks assigned for this cer_type */
		cer_type = cer_frm_get_type(frm);
		cer_payload = cer_frm_get_payload(frm);
		LIST_FOREACH(&cer_if->list_rx_subscribers, le) {
			struct cer_rx_handler *handler = list_ledata(le);
			if (handler->cer_type == cer_type) {
				handler->callback(handler->priv,
						cer_payload, frm->payload_size);
			}
		}

		cli();
		frm->ready = 0;
		sei();
	}
}


/**
 * Push byte in cerium frame
 * @param frm - pointer to Cerium frame
 * @param byte
 * @return 0 if ok
 */
static inline int ber_frm_push_byte(struct cer_frm *frm, u8 byte)
{
	if (frm->payload_size >= CER_FRM_MTU)
		return -ENOSPC;

	frm->data[frm->payload_size++] = byte;
	return 0;
}


/**
 * IRQ callback for receive one byte from UART
 * @param cer_if - cer interface
 * @param rxb - received byte
 */
static void cer_receiver(struct cer_if *cer_if, u8 rxb)
{
	int rc;
	struct cer_frm *frm = cer_if->curr_rx_frm;

	if (rxb == CER_FRM_MARK && !cer_if->mark_detected) {
		cer_if->mark_detected = 1;
		if (!cer_if->rx_timeout)
			return;
	}

	/* if BER_MARK symbol receive at second time - create new frame */
	if (cer_if->mark_detected && (rxb != CER_FRM_MARK || cer_if->rx_timeout)) {
		cer_if->mark_detected = 0;

		if (frm->payload_size >= (CER_FRM_HDR_SIZE + CER_FRM_FCB_SIZE)) {
			frm->payload_size -= (CER_FRM_HDR_SIZE + CER_FRM_FCB_SIZE);
			cer_if->cnt_frm_rx++;
			frm->ready = 1;
		}

		frm = get_empty_frm(cer_if, 0);
		if (!frm) {
			cer_if->cnt_not_memory++;
			return;
		}

		if (rxb == CER_FRM_MARK)
			return;
	}

	/* new byte is added to the frame */
	cer_if->mark_detected = 0;
	cer_if->rx_timeout = 0;
	rc = ber_frm_push_byte(frm, rxb);
	if (rc) {
		/* if cerium frame is full,
		 * drop his and create new frame */
		frm->payload_size = 0;
		cer_if->cnt_frm_rx_overflow++;
		ber_frm_push_byte(frm, rxb);
	}
	cer_if->cnt_rx_bytes++;
}

/**
 * IRQ callback for transmit one byte into UART
 * @param cer_if - cer interface
 */
static void cer_transmitter(struct cer_if *cer_if)
{
	struct cer_frm *frm = cer_if->curr_tx_frm;

	if (cer_if->tx_cnt >= frm->payload_size) {
		frm->ready = 0;
		cer_if->tx_cnt = 0;
	}

	if (!frm->ready)
		frm = get_filled_frm(cer_if, 1);
	if (!frm) {
		cer_if->tx_running = 0;
		return;
	}

	usart_send_byte(&cer_if->uart, frm->data[cer_if->tx_cnt++]);
	cer_if->tx_running = 1;
}


/**
 * Register Cerium interface
 * @param cer_if - pointer to allocated memory for Cerium interface
 * @param uart_id - UART controller id
 * @param uart_speed - speed in bod per second
 * @return 0 if ok
 */
int cerium_register(struct cer_if *cer_if, int uart_id, int uart_speed)
{
	struct uart *uart = &cer_if->uart;
	int rc;

	memset(cer_if, 0, sizeof (*cer_if));
	cer_if->curr_rx_frm = cer_if->processing_rx_frm = cer_if->rx_frames;

	uart->chip_id = uart_id;
	uart->baud_rate = uart_speed;
	uart->rx_int = (void (*)(void *, u8))cer_receiver;
	uart->udre_int = (void (*)(void *))cer_transmitter;

	cer_if->timer.devisor = 1;
	cer_if->timer.priv = cer_if;
	cer_if->timer.handler = (void (*)(void *))cer_if_timer;
	sys_timer_add_handler(&cer_if->timer);

	cer_if->wrk.priv = cer_if;
	cer_if->wrk.handler = (void (*)(void *))cer_if_rx_work;
	sys_idle_add_handler(&cer_if->wrk);

	rc = usart_init(uart);
	if (rc)
		return rc;
	return 0;
}

/**
 * add received frame handler
 * @param cer_if - cer interface
 * @param handler - allocated handler structure
 */
void cerium_add_rx_handler(struct cer_if *cer_if, struct cer_rx_handler *handler)
{
	cli();
	list_append(&cer_if->list_rx_subscribers, &handler->le, handler);
	sei();
}

/**
 * Send frame into a RS-485 bus
 * @param cer_if - pointer to allocated memory
 * @param type - cer_type
 * @param data - payload data pointer
 * @param size - size of payload data
 * @return 0 if ok
 */
int cerium_send_frm(struct cer_if *cer_if, u8 type, u8 *data, u8 size)
{
	struct cer_frm *frm;
	int cnt, result_cnt = 0, frm_data_cnt = 0;
	u8 frm_data[CER_FRM_MTU * 2];
	u16 *crc;

	cli();
	frm = get_empty_frm(cer_if, 1);
	sei();
	if (!frm)
		return -ENOSPC;

	frm_data[frm_data_cnt++] = type;
	memcpy(frm_data + frm_data_cnt, data, size);
	frm_data_cnt += size;

	/* added FCS */
	crc = (u16 *)(frm_data + size);
	*crc = crc16(frm_data, size);

	/* doubling CER_FRM_MARK bytes in data */
	for (cnt = 0; cnt < frm_data_cnt; cnt++) {
		frm->data[result_cnt++] = frm_data[cnt];
		if (frm_data[cnt] == CER_FRM_MARK)
			frm->data[result_cnt++] = frm_data[cnt];
	}

	/* added frame separator */
	frm->data[result_cnt++] = CER_FRM_MARK;
	frm->payload_size = result_cnt;
	cli();
	frm->ready = 1;
	cer_if->rx_ready = 1;

	/* start transmitter */
	if (!cer_if->tx_running)
		cer_transmitter(cer_if);
	sei();

	return 0;
}

