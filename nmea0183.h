/*
 * nmea0183.h
 *
 *  Created on: 03 янв. 2017 г.
 *      Author: Michail Kurochkin
 */
#ifndef NMEA0183_H_
#define NMEA0183_H_

#include "types.h"
#include "uart.h"
#include "idle.h"

#define NMEA_MSG_RING_SIZE 8 /* Number of RX/TX ring buffers */

enum nmea_talkers_identifiers {
	NMEA_TI_UNDEFINED = -1,
	NMEA_TI_IO, // IO (Input/Output) sender
	NMEA_TI_PC // PC (Personal Computer) sender
};

enum nmea_sentence_identifiers {
	NMEA_SI_UNDEFINED = -1,
	NMEA_SI_RWS, // (Relay Write State) Write Relay on/off
	NMEA_SI_RRS, // (Relay Read State) Read Relay state
	NMEA_SI_RIP, // (Request Input Port) Request input port state
	NMEA_SI_AIP, // (Action on Input Port)
};

struct nmea_msg {
	char msg_buf[48];
	int buf_len;
	bool ready :1;
	enum nmea_talkers_identifiers ti;
	enum nmea_sentence_identifiers si;
	char argv[5][8]; /* parsed arguments */
	u8 argc; /* number parsed arguments */
};

struct nmea_if {
	struct uart uart; /* UART controller descriptor */
	struct sys_work wrk; /* work descriptor */
	volatile u8 tx_cnt; /* transmit byte counter in current buffer (curr_tx_msg) */
	struct nmea_msg rx_messages[NMEA_MSG_RING_SIZE]; /* ring buffer for rx frames */
	struct nmea_msg tx_messages[NMEA_MSG_RING_SIZE]; /* ring buffer for tx frames */
	struct nmea_msg *curr_rx_msg; /* current receiving rx buffer */
	struct nmea_msg *processing_rx_msg; /* current processing rx buffer */
	struct nmea_msg *curr_tx_msg; /* current processing tx buffer */
	struct nmea_msg *processing_tx_msg; /* current processing tx buffer */
	void (*rx_msg_handler)(struct nmea_msg *);

	volatile u8 rx_cr: 1; /* Receive '\r' symbol */
	volatile u8 tx_running : 1; /* flag is set when transmitter is running */
	volatile u8 rx_ready: 1; /* flag is set, if new frame was received */

	u16 cnt_msg_rx;
	u16 cnt_rx_ring_overflow;
	u16 cnt_tx_ring_overflow;
	u16 cnt_checksumm_err;
	u16 cnt_rx_bytes;
};

int nmea_register(struct nmea_if *nmea_if, int uart_id, int uart_speed,
				void (*rx_msg_handler)(struct nmea_msg *));
int nmea_send_msg(struct nmea_if *nmea_if, struct nmea_msg *sending_msg);

#endif /* NMEA0183_H_ */
