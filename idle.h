/*
 * idle.h
 *
 *  Created on: 07 июля 2016 г.
 *      Author: Michail Kurochkin
 */
#ifndef IDLE_H_
#define IDLE_H_

#include "list.h"

struct sys_work {
	void *priv;
	void (*handler)(void *);
// Private:
	struct le le;
};

void idle(void);
void sys_idle_add_handler(struct sys_work *wrk);

#endif /* IDLE_H_ */
