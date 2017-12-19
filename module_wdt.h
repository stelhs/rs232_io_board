/*
 * module_wdt.h
 *
 *  Created on: 06 янв. 2017 г.
 *      Author: Michail Kurochkin
 */
#ifndef MODULE_IO_WDT_H_
#define MODULE_IO_WDT_H_

#include "idle.h"
#include "sys_timer.h"
#include "types.h"
#include "nmea0183.h"
#include "module_io.h"

enum mwdt_reboot_states {
	MWDT_DISABLE,
	MWDT_POWER_DOWN,
	MWDT_WAIT_POWER,
	MWDT_PRESS_POWER_BUTT,
	MWDT_WAIT_BOOTING,
	MWDT_READY,
};

struct mwdt {
	struct sys_timer timer;
	struct sys_work wrk;
	struct mio *io;

	volatile u8 timeout_elapsed :1;
	volatile u16 counter; /* WDT counter */

	enum mwdt_reboot_states state;
	struct nmea_if *nmea_if;
	struct nmea_subsriber ns;
};

int m_wdt_init(struct mwdt *wdt, struct nmea_if *nmea_if, struct mio *io);
void m_wdt_enable(struct mwdt *wdt);
void m_wdt_disable(struct mwdt *wdt);
void m_wdt_reset(struct mwdt *wdt);

#endif /* MODULE_IO_WDT_H_ */
