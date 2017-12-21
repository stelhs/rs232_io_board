/*
 * module_wdt.c
 *
 *  Created on: 29.12.2016
 *      Author: Michail Kurochkin
 */

#include <string.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include "board.h"
#include "types.h"
#include "idle.h"
#include "sys_timer.h"
#include "module_io.h"
#include "module_wdt.h"


#define MWDT_TIMER_RESOLUTION 200 /* Timer resolution in milliseconds */

#define WDT_5V_IN_NUM 1
#define WDT_POWER_OUT_NUM 1
#define WDT_POWER_BUTT_OUT_NUM 2
#define WDT_POWER_DOWN_INTERVAL (3000 / MWDT_TIMER_RESOLUTION) /* Power down server interval */
#define WDT_WAITING_POWER_INTERVAL (3000 / MWDT_TIMER_RESOLUTION) /* Server power detect interval after server power on. */
#define WDT_POWER_BUTTON_PRESSING_INTERVAL (600 / MWDT_TIMER_RESOLUTION) /* Power button pressed interval */
#define WDT_TIMEOUT_WHILE_BOOTING ((1000L * 60 * 5) / MWDT_TIMER_RESOLUTION) /* Server booting time */
#define WDT_TIMEOUT (30000 / MWDT_TIMER_RESOLUTION)/* WDT timeout */


static void server_set_power_down(struct mwdt *wdt, u8 mode)
{
	usio_relay_set_state(wdt->io, WDT_POWER_OUT_NUM, mode);
}

static void server_set_power_button(struct mwdt *wdt, u8 mode)
{
	usio_relay_set_state(wdt->io, WDT_POWER_BUTT_OUT_NUM, mode);
}


static void timer_handler(struct mwdt *wdt)
{
	if (wdt->timeout_elapsed || (!wdt->counter))
		return;

	wdt->counter--;
	if (wdt->counter > 0)
		return;

	wdt->timeout_elapsed = 1;
}


static void m_wdt_server_reboot(struct mwdt *wdt)
{
	printf("m_wdt_server_reboot\r\n");
	server_set_power_down(wdt, ON);
	cli();
	wdt->state = MWDT_POWER_DOWN;
	wdt->counter = WDT_POWER_DOWN_INTERVAL;
	sei();
}


static void work_handler(struct mwdt *wdt)
{
	if (wdt->state == MWDT_DISABLE)
		return;

	switch (wdt->state) {
	case MWDT_POWER_DOWN:
		if (!wdt->timeout_elapsed)
			return;

		wdt->timeout_elapsed = 0;
		server_set_power_down(wdt, OFF);
		wdt->state = MWDT_WAIT_POWER;
		cli();
		wdt->counter = WDT_WAITING_POWER_INTERVAL;
		sei();
		return;

	case MWDT_WAIT_POWER:
		if (usio_get_input_state(wdt->io, WDT_5V_IN_NUM)) {
			wdt->state = MWDT_WAIT_BOOTING;
			cli();
			wdt->counter = WDT_TIMEOUT_WHILE_BOOTING;
			sei();
			return;
		}

		if (wdt->timeout_elapsed) {
			wdt->timeout_elapsed = 0;
			server_set_power_button(wdt, ON);
			wdt->state = MWDT_PRESS_POWER_BUTT;
			cli();
			wdt->counter = WDT_POWER_BUTTON_PRESSING_INTERVAL;
			sei();
			return;
		}
		return;

	case MWDT_PRESS_POWER_BUTT:
		if (!wdt->timeout_elapsed)
			return;

		wdt->timeout_elapsed = 0;
		server_set_power_button(wdt, OFF);
		wdt->state = MWDT_WAIT_POWER;
		cli();
		wdt->counter = WDT_WAITING_POWER_INTERVAL;
		sei();
		return;


	case MWDT_WAIT_BOOTING:
		if (!wdt->timeout_elapsed)
			return;

		wdt->timeout_elapsed = 0;
		wdt->state = MWDT_READY;
		cli();
		wdt->counter = WDT_TIMEOUT;
		sei();
		return;

	case MWDT_READY:
		if (!wdt->timeout_elapsed)
			return;

		wdt->timeout_elapsed = 0;
		printf("m_wdt_server_reboot\r\n");
		m_wdt_server_reboot(wdt);
		return;
	default:
		return;
	}
}

static int send_wdt_state(struct mwdt *wdt, int sender_id)
{
	struct nmea_msg msg;

	nmea_msg_reset(&msg);
	msg.ti = NMEA_TI_IO;
	msg.si = NMEA_SI_WDS;
	sprintf(msg.argv[1], "%d", sender_id);
	sprintf(msg.argv[2], "%d", wdt->state);
	msg.argc = 2;
	return nmea_send_msg(wdt->nmea_if, &msg);
}


void m_wdt_enable(struct mwdt *wdt)
{
	static u8 first = 1;

	if (!usio_get_input_state(wdt->io, WDT_5V_IN_NUM)) {
		m_wdt_server_reboot(wdt);
		return;
	}

	if (first) {
		cli();
		wdt->state = MWDT_WAIT_BOOTING;
		wdt->counter = WDT_TIMEOUT_WHILE_BOOTING;
		sei();
		first = 0;
		return;
	}

	cli();
	wdt->state = MWDT_READY;
	wdt->counter = WDT_TIMEOUT;
	sei();
}

void m_wdt_disable(struct mwdt *wdt)
{
	if (wdt->state == MWDT_DISABLE)
		return;

	if (wdt->state < MWDT_WAIT_BOOTING) {
		server_set_power_down(wdt, OFF);
		server_set_power_button(wdt, OFF);
	}

	cli();
	wdt->state = MWDT_DISABLE;
	wdt->counter = 0;
	sei();
}


void m_wdt_reset(struct mwdt *wdt)
{
	if (wdt->state == MWDT_DISABLE)
		return;

	cli();
	wdt->counter = WDT_TIMEOUT;
	sei();
}


static void nmea_rx_msg(struct nmea_msg *msg, struct mwdt *wdt)
{
	int state;

	if (msg->ti != NMEA_TI_PC)
		return;

	switch (msg->si) {
	case NMEA_SI_WDC:
		if (msg->argc < 3)
			break;

		sscanf(msg->argv[2], "%d", &state);
		if (state)
			m_wdt_enable(wdt);
		else
			m_wdt_disable(wdt);

		send_wdt_state(wdt, msg->sender_id);
		break;

	case NMEA_SI_WDS: // Get curent WDS state
		if (msg->argc < 2)
			break;

		send_wdt_state(wdt, msg->sender_id);
		break;

	case NMEA_SI_WRS:
		m_wdt_reset(wdt);
		break;

	default:
		return;
	}
}


int m_wdt_init(struct mwdt *wdt, struct nmea_if *nmea_if, struct mio *io)
{
	memset(wdt, 0, sizeof(*wdt));
	wdt->nmea_if = nmea_if;
	wdt->io = io;
	wdt->state = MWDT_READY;

	wdt->ns.rx_callback = (void (*)(struct nmea_msg *, void *))nmea_rx_msg;
	wdt->ns.priv = wdt;
	nmea_add_subscriber(nmea_if, &wdt->ns);

	wdt->wrk.handler = (void (*)(void *))work_handler;
	wdt->wrk.priv = wdt;
	sys_idle_add_handler(&wdt->wrk);

	wdt->timer.devisor = MWDT_TIMER_RESOLUTION;
	wdt->timer.priv = wdt;
	wdt->timer.handler = (void (*)(void *))timer_handler;
	sys_timer_add_handler(&wdt->timer);

	m_wdt_enable(wdt);
	return 0;
}
