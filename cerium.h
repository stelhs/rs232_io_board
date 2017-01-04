/*
 * cerium.h
 *
 *  Created on: 02 янв. 2017 г.
 *      Author: Michail Kurochkin
 */
#ifndef CERIUM_H_
#define CERIUM_H_

#include "types.h"
#include "uart.h"
#include "sys_timer.h"
#include "idle.h"
#include "list.h"

#define CER_FRM_MTU 16 /* Frame MTU size */
#define CER_FRM_MARK 0x53 /* Frame's delimiter Marker value */
#define CER_FRM_HDR_SIZE 1 /* Header size */
#define CER_FRM_FCB_SIZE 2 /* Size of FCB tail (use crc16) */
#define CER_RING_SIZE 8 /* Number of RX/TX ring buffer in cer_frames */


/**
 * Cerium frame descriptor
 */
struct cer_frm {
	u8 payload_size; /* frame payload size */
	u8 ready: 1; /* for RX: complete received frame */
	u8 data[CER_FRM_MTU * 2]; /* frame memory */
};

/**
 * Cerium interface descriptor
 */
struct cer_if {
	struct uart uart; /* UART descriptor */
	struct sys_timer timer; /* timer descriptor */
	struct sys_work wrk; /* work descriptor */
	volatile u8 rx_timeout: 1; /* rx timeout stay to one if bytes was not received longly */
	volatile u8 rx_timeout_counter; /* timeout counter per 1ms */
	volatile u8 rx_ready: 1; /* flag is set to if new frame has been received */
	volatile u8 tx_ready: 1; /* flag is set to if frame has been filled and waiting for transmit */
	volatile u8 tx_running : 1; /* flag is set when transmitter is running */
	u8 mark_detected: 1; /* delimiter marker detected */
	volatile u8 tx_cnt; /* trannsmit byte counter in current frame (curr_tx_frm) */
	struct cer_frm rx_frames[CER_RING_SIZE]; /* ring buffer for rx frames */
	struct cer_frm tx_frames[CER_RING_SIZE]; /* ring buffer for tx frames */
	struct cer_frm *curr_rx_frm; /* current receiving rx buffer */
	struct cer_frm *processing_rx_frm; /* current processing rx buffer */
	struct cer_frm *curr_tx_frm; /* current processing tx buffer */
	struct cer_frm *processing_tx_frm; /* current processing tx buffer */

	struct list list_rx_subscribers;

	u16 cnt_rx_bytes;
	u16 cnt_frm_rx;
	u16 cnt_frm_rx_overflow;
	u16 cnt_frm_rx_crc_err;
	u16 cnt_not_memory;
};


/**
 * Beryllium frame header structure
 */
struct cer_frame_hdr {
	union {
		u8 start_data; /* first byte of start frame data */
		struct {
			u8 cer_type;
			u8 start_payload; /* first byte of payload data */
		};
	};
};

static inline u8 cer_frm_get_type(struct cer_frm *frm)
{
	struct cer_frame_hdr *hdr = (struct cer_frame_hdr *)frm->data;
	return hdr->cer_type;
}

/**
 * Get pointer to payload data of beryllium frame
 */
static inline u8 *cer_frm_get_payload(struct cer_frm *frm)
{
	struct cer_frame_hdr *hdr = (struct cer_frame_hdr *)frm->data;
	return &hdr->start_payload;
}

/**
 * RX event subscriber descriptor
 */
struct cer_rx_handler {
	u8 cer_type;
	void *priv;
	void (*callback)(void *priv, u8 *data, u8 size);
// Private:
	struct le le;
};


int cerium_register(struct cer_if *cer_if, int uart_id, int uart_speed);
int cerium_send_frm(struct cer_if *cer_if, u8 type, u8 *data, u8 size);
void cerium_add_rx_handler(struct cer_if *cer_if, struct cer_rx_handler *handler);

#endif /* CERIUM_H_ */
