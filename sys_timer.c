/*
 * sys_timer.c
 *
 *  Created on: 07 июля 2016 г.
 *      Author: Michail Kurochkin
 */

#include <avr/interrupt.h>
#include "types.h"
#include "list.h"
#include "sys_timer.h"


static struct list list_subscribers = LIST_INIT;

ISR(TIMER2_COMP_vect)
{
	struct le *le;

	LIST_FOREACH(&list_subscribers, le) {
		struct sys_timer *timer = list_ledata(le);

		timer->counter++;
		if (timer->counter == timer->devisor) {
			timer->handler(timer->priv);
			timer->counter = 0;
		}
	}
}

void sys_timer_add_handler(struct sys_timer *timer)
{
	cli();
	timer->counter = 0;
	list_append(&list_subscribers, &timer->le, timer);
	sei();
}

/* configure timer2 to frequency 1000Hz */
#define TIMER2_DELAY (u8)((u32)F_CPU / 1000 / (2 * 32) - 1)
void sys_timer_init(void)
{
	OCR2 = TIMER2_DELAY;
	TCCR2 = 0b011;
	TIFR |= OCF2;
	TIMSK |= _BV(OCIE2);
}
