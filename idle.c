/*
 * idle.c
 *
 *  Created on: 07 июля 2016 г.
 *      Author: Michail Kurochkin
 */

#include "types.h"
#include "list.h"
#include "idle.h"
#include "board.h"

static struct list list_subscribers = LIST_INIT;

/**
 * Must be placed into main cycle
 */
void idle(void)
{
	static struct le *le;

	LIST_FOREACH(&list_subscribers, le) {
		struct sys_work *wrk = list_ledata(le);
		wrk->handler(wrk->priv);
	}
}

void sys_idle_add_handler(struct sys_work *wrk)
{
	list_append(&list_subscribers, &wrk->le, wrk);
}
