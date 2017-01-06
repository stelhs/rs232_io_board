/*
 * io_wdt.h
 *
 *  Created on: 06 янв. 2017 г.
 *      Author: Michail Kurochkin
 */
#ifndef MODULE_IO_WDT_H_
#define MODULE_IO_WDT_H_

#include "idle.h"
#include "sys_timer.h"
#include "types.h"
#include "module_io.h"

enum io_wdt_reboot_states {
	IO_WDT_POWER_DOWN,
	IO_WDT_WAIT_POWER,
	IO_WDT_PRESS_POWER_BUTT,
	IO_WDT_WAIT_BOOTING,
	IO_WDT_READY,
};

struct io_module_wdt {
	struct sys_timer timer;
	struct sys_work wrk;
	struct io_module *io;

	volatile u8 enable :1; /* WDT enable or disable */
	volatile u8 timeout_elapsed :1;
	volatile u16 counter; /* WDT counter */

	enum io_wdt_reboot_states state;
} wdt;

int module_io_wdt_init(struct io_module_wdt *wdt, struct io_module *io);
void io_wdt_enable(struct io_module_wdt *wdt);
void io_wdt_disable(struct io_module_wdt *wdt);
void io_wdt_server_reboot(struct io_module_wdt *wdt);
void io_wdt_reset(struct io_module_wdt *wdt);

#endif /* MODULE_IO_WDT_H_ */
