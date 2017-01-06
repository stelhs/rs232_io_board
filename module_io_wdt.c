/*
 * module_io_wdt.c
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
#include "module_io_wdt.h"


#define IO_TIMER_RESOLUTION 200 /* Timer resolution in milliseconds */

#define WDT_5V_IN_NUM 1
#define WDT_POWER_OUT_NUM 1
#define WDT_POWER_BUTT_OUT_NUM 2
#define WDT_POWER_DOWN_INTERVAL (3000 / IO_TIMER_RESOLUTION) /* Power down server interval */
#define WDT_WAITING_POWER_INTERVAL (5000 / IO_TIMER_RESOLUTION) /* Server power detect interval after server power on. */
#define WDT_POWER_BUTTON_PRESSING_INTERVAL (600 / IO_TIMER_RESOLUTION) /* Power button pressed interval */
#define WDT_TIMEOUT_WHILE_BOOTING ((1000L * 60 * 5) / IO_TIMER_RESOLUTION) /* Server booting time */
#define WDT_TIMEOUT 30000 /* WDT timeout */


static void server_set_power_down(struct io_module_wdt *wdt, u8 mode)
{
	io_output_set_state(wdt->io, WDT_POWER_OUT_NUM, mode);
}

static void server_set_power_button(struct io_module_wdt *wdt, u8 mode)
{
	io_output_set_state(wdt->io, WDT_POWER_BUTT_OUT_NUM, mode);
}


static void timer_handler(struct io_module_wdt *wdt)
{
	if (!wdt->enable)
		return;

	if (wdt->timeout_elapsed || (!wdt->counter))
		return;

	wdt->counter--;
	if (wdt->counter > 0)
		return;

	wdt->timeout_elapsed = 1;
}


static void work_handler(struct io_module_wdt *wdt)
{
	switch (wdt->state) {
	case IO_WDT_POWER_DOWN:
		if (!wdt->timeout_elapsed)
			return;

		wdt->timeout_elapsed = 0;
		server_set_power_down(wdt, OFF);
		wdt->state = IO_WDT_WAIT_POWER;
		cli();
		wdt->counter = WDT_WAITING_POWER_INTERVAL;
		sei();
		return;

	case IO_WDT_WAIT_POWER:
		if (io_get_input_state(wdt->io, WDT_5V_IN_NUM)) {
			wdt->state = IO_WDT_WAIT_BOOTING;
			cli();
			wdt->counter = WDT_TIMEOUT_WHILE_BOOTING;
			sei();
			return;
		}

		if (wdt->timeout_elapsed) {
			wdt->timeout_elapsed = 0;
			server_set_power_button(wdt, ON);
			wdt->state = IO_WDT_PRESS_POWER_BUTT;
			cli();
			wdt->counter = WDT_POWER_BUTTON_PRESSING_INTERVAL;
			sei();
			return;
		}
		return;

	case IO_WDT_PRESS_POWER_BUTT:
		if (!wdt->timeout_elapsed)
			return;

		wdt->timeout_elapsed = 0;
		server_set_power_button(wdt, OFF);
		wdt->state = IO_WDT_WAIT_POWER;
		cli();
		wdt->counter = WDT_WAITING_POWER_INTERVAL;
		sei();
		return;


	case IO_WDT_WAIT_BOOTING:
		if (!wdt->timeout_elapsed)
			return;

		wdt->timeout_elapsed = 0;
		wdt->state = IO_WDT_READY;
		cli();
		wdt->counter = WDT_TIMEOUT;
		sei();
		return;

	case IO_WDT_READY:
		if (!wdt->timeout_elapsed)
			return;

		wdt->timeout_elapsed = 0;
		io_wdt_server_reboot(wdt);
		return;
	}
}



void io_wdt_enable(struct io_module_wdt *wdt)
{
	if (wdt->enable)
		return;

	cli();
	wdt->enable = 1;
	sei();

	if (!io_get_input_state(wdt->io, WDT_5V_IN_NUM)) {
		io_wdt_server_reboot(wdt);
		return;
	}

	cli();
	wdt->state = IO_WDT_WAIT_BOOTING;
	wdt->counter = WDT_TIMEOUT_WHILE_BOOTING;
	sei();
}

void io_wdt_disable(struct io_module_wdt *wdt)
{
	cli();
	wdt->enable = 0;
	sei();
	server_set_power_down(wdt, OFF);
	server_set_power_button(wdt, OFF);
}


void io_wdt_server_reboot(struct io_module_wdt *wdt)
{
	server_set_power_down(wdt, ON);
	wdt->state = IO_WDT_POWER_DOWN;
	wdt->counter = WDT_POWER_DOWN_INTERVAL;
}

void io_wdt_reset(struct io_module_wdt *wdt)
{
	cli();
	wdt->counter = WDT_TIMEOUT;
	sei();
}


int module_io_wdt_init(struct io_module_wdt *wdt, struct io_module *io)
{
	memset(wdt, 0, sizeof(*wdt));
	wdt->io = io;
	wdt->state = IO_WDT_READY;
	io->wdt = wdt;

	wdt->wrk.handler = (void (*)(void *))work_handler;
	wdt->wrk.priv = wdt;
	sys_idle_add_handler(&wdt->wrk);

	wdt->timer.devisor = IO_TIMER_RESOLUTION;
	wdt->timer.priv = wdt;
	wdt->timer.handler = (void (*)(void *))timer_handler;
	sys_timer_add_handler(&wdt->timer);

	io_wdt_enable(wdt);
	return 0;
}
