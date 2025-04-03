/*
 * app.h
 *
 *  Created on: Sep 25, 2013
 *      Author: shved
 */

#ifndef APP_H_
#define APP_H_

#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include <osip2/osip.h>

#include "lib/proto/sip/sip.h"

#include "net.h"

#define SIP_DEFAULT_VERSION		"SIP/2.0"
#define SIP_DEFAULT_PORT		"5060"
#define SIP_DEFAULT_EXPIRES		3600
#define SIP_DEFAULT_MAX_FORWARD	"70"

#define SIP_METHOD_ACK			"ACK"
#define SIP_METHOD_BYE			"BYE"
#define SIP_METHOD_CANCEL		"CANCEL"
#define SIP_METHOD_INFO			"INFO"
#define SIP_METHOD_INVITE		"INVITE"
#define SIP_METHOD_MESSAGE		"MESSAGE"
#define SIP_METHOD_NOTIFY		"NOTIFY"
#define SIP_METHOD_OPTIONS		"OPTIONS"
#define SIP_METHOD_PRACK 		"PRACK"
#define SIP_METHOD_PUBLISH		"PUBLISH"
#define SIP_METHOD_REFER		"REFER"
#define SIP_METHOD_REGISTER		"REGISTER"
#define SIP_METHOD_SUBSCRIBE	"SUBSCRIBE"
#define SIP_METHOD_UPDATE		"UPDATE"

#define SIP_DEFAULT_ALLOW		SIP_METHOD_INVITE ", " SIP_METHOD_ACK ", " SIP_METHOD_SUBSCRIBE ", " \
								SIP_METHOD_CANCEL ", " SIP_METHOD_BYE ", " SIP_METHOD_OPTIONS ", " \
								SIP_METHOD_NOTIFY ", " SIP_METHOD_UPDATE

#define __type_name(T) 		{(T), #T}
#define __type_name_END		{ 0, NULL }

void app_set_hostname(const app_t *, const char *);

struct app {
    const char *name;
    char *host;
    char *hostname;
    char *port;

    int (*acquire)(net_event_t *);

    int (*release)(net_event_t *);

    int (*execute)(net_event_t *);

    int (*timeout)(net_event_t *);

    int (*connect)(net_event_t *, const va_list);

    struct {
        map_del_t destroy;
        map_cmp_t compare;
    } map[1];

    struct timeval delay;
};

/* sip.c export */
int sip_init();

int sip_call(osip_transaction_t *, osip_event_t *);

int sip_loop(osip_event_t *, void *, ...);

int sip_get_online(const char *);

void sip_free();

void sip_delete(osip_transaction_t *);

void sip_destroy(void *);

void sip_dump(const char *, const char *, unsigned);

void sip_finalize(osip_message_t *, int);

void sip_finalize_failure(int);

void sip_proxy(osip_message_t *, Sip__Message_Closure, void *);

/* cmd.c export */
int cmd_init();

void cmd_free();

int cmd_initiate_registrate(osip_transaction_t *, osip_message_t *);

int cmd_initiate_invite(osip_transaction_t *, osip_message_t *);

int cmd_initiate_option(osip_transaction_t *, osip_message_t *);

int cmd_initiate_cancel(osip_transaction_t *, osip_message_t *);

int cmd_initiate_bye(osip_transaction_t *, osip_message_t *);

int cmd_initiate_info(osip_transaction_t *, osip_message_t *);

int cmd_initiate_subscribe(osip_transaction_t *, osip_message_t *);

int cmd_initiate_notify(osip_transaction_t *, osip_message_t *);

int cmd_finalize(const osip_message_t *, Sip__Message_Closure, void *);

/* dns.c export */
int dns_init();

void dns_free();

int dns_lookup(const char *, void (*)(void *, const char *), void *);

int dns_is_resolved(const char *);

void dns_callback(void *, const char *);

extern const app_t __g_app_SIP;
extern const app_t __g_app_CMD;
extern const app_t __g_app_DNS;

extern char *strncasestr(const char *, const char *, size_t);

#endif /* APP_H_ */
