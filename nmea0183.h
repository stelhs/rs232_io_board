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
#define NMEA_MSG_MAX_LEN 48 /* NMEA message max length */

enum nmea_talkers_identifiers {
	NMEA_TI_UNDEFINED = -1,
	NMEA_TI_IO, // IO (Input/Output) sender
	NMEA_TI_PC // PC (Personal Computer) sender
};

enum nmea_sentence_identifiers {
	NMEA_SI_UNDEFINED = -1,
	NMEA_SI_RWS, // (Relay Write State) Write Relay on/off
	NMEA_SI_RRS, // (Request Relay Read State) Read Relay state
	NMEA_SI_RIP, // (Request Input Port) Request input port state
	NMEA_SI_AIP, // (Action on Input Port)
	NMEA_SI_SOP, // (State Output Port)
	NMEA_SI_WDS, // (WDT State ON/OFF)
	NMEA_SI_WRS, // (WDT Reset)
};

struct nmea_msg {
	char msg_buf[NMEA_MSG_MAX_LEN];
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
	void (*rx_msg_handler)(struct nmea_msg *, void *);
	void *priv; /* private data associated with this interface */

	volatile u8 rx_carry: 1; /* Receive '\r' or '\n' symbol */
	volatile u8 rx_ready: 1; /* flag is set, if new frame was received */

	u16 cnt_msg_rx;
	u16 cnt_rx_ring_overflow;
	u16 cnt_tx_ring_overflow;
	u16 cnt_checksumm_err;
	u16 cnt_rx_bytes;
	u16 cnt_rx_overflow;
};

int nmea_register(struct nmea_if *nmea_if, int uart_id, int uart_speed,
			void (*rx_msg_handler)(struct nmea_msg *, void *));
void nmea_msg_reset(struct nmea_msg *msg);
int nmea_send_msg(struct nmea_if *nmea_if, struct nmea_msg *sending_msg);

#endif /* NMEA0183_H_ */
