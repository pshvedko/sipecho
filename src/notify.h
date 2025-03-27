/*
 * notify.h
 *
 *  Created on: Nov 28, 2013
 *      Author: shved
 */

#ifndef NOTIFY_H_
#define NOTIFY_H_

#include <stdio.h>

#include "lib/proto/sip/message.pb-c.h"

#define NOTIFY_DEFAULT_VIA_HOSTNAME	"127.0.0.1"
#define NOTIFY_DEFAULT_VIA_VERSION	"2.0"
#define NOTIFY_DEFAULT_VIA_PROTOCOL	"INT"

typedef struct notify Sip__Notify;

Sip__Notify * notify_new(int (*)(const Sip__Type__Address *, void *), void *);

int notify_update(Sip__Notify *, const Sip__Type__Address *, const Sip__Type__Address *,
		const Sip__Type__Address *, const Sip__Type__CallId *, int, void (*)(const Sip__Query *, void *),
		void *);

int notify_delete(Sip__Notify *, const Sip__Type__Address *, const Sip__Type__Address *,
		const Sip__Type__Address *);

int notify_change(Sip__Notify *, const Sip__Type__Address *, int, void (*)(const Sip__Query *, void *), void *);

void notify_free(Sip__Notify *);

#endif /* NOTIFY_H_ */
