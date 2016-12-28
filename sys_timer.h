/*
 * sys_timer.h
 *
 *  Created on: 07 июля 2016 г.
 *      Author: Michail Kurochkin
 */
#ifndef SYS_TIMER_H_
#define SYS_TIMER_H_

#include "list.h"

struct sys_timer {
	int devisor;
	void *priv;
	void (*handler)(void *);
// Private:
	struct le le;
	int counter;
};

void sys_timer_add_handler(struct sys_timer *timer);
void sys_timer_init(void);

#endif /* SYS_TIMER_H_ */
