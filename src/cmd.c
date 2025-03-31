/*
 * acp.c
 *
 *  Created on: Oct 7, 2013
 *      Author: shved
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "lib/common/map.h"
#include "lib/proto/sip/sip.h"
#include "lib/proto/transport.h"

#include "app.h"
#include "notify.h"
#include "log.h"

static net_event_t *__con = NULL;
static transport_t *__cmd = NULL;

static char hostname[];

#define XXX
#ifndef XXX

/**
 * XXX
 */
static int cmd_status_notify(const Sip__Type__Address *who, void *foo) {

    if (!who || !who->url)
        return 0;
    if (!who->url->scheme || !who->url->username || !who->url->host)
        return 0;

    char name[512];

    snprintf(name, sizeof(name), "%s:%s@%s", who->url->scheme, who->url->username, who->url->host);

    return sip_get_online(name);
}

/**
 * XXX
 */
static void cmd_closed_notify(const Sip__Answer *result, void *opaque) {

    if (!result)
        return;

    log_debug("%s: %i %p", __PRETTY_FUNCTION__, result->response, opaque);
}

/**
 * XXX
 */
static void cmd_submit_notify(const Sip__Query *notice, void *foo) {

    sip__notify(foo, notice, &cmd_closed_notify, 0);
}

/**
 * XXX
 */
static void cmd_maintain_registrate(Sip_Service *foo, const Sip__Query *query, Sip__Answer_Closure closure,
        void *opaque) {

    if (!query || !query->head || !query->request) {

        closure(NULL, opaque);
        return;
    }
    log_debug("%s: %p %s", __PRETTY_FUNCTION__, query->request, query->head->version);

    Sip__Answer result[1];

    sip__answer__init(result);

    result->head = query->head;
    result->response = SIP_OK;

    closure(result, opaque);

    int n = query->head->n_other;
    while (n--) {
        if (strcasecmp("Expires", query->head->other[n]->name ?query->head->other[n]->name: "") == 0) {
            if (strcmp("0", query->head->other[n]->value ? query->head->other[n]->value: "") != 0) {
                notify_change(__notify, query->head->to, 1, &cmd_submit_notify, foo);
                return;
            }
        }
    }
    int j = query->head->n_contact;
    while (j--)
        notify_delete(__notify, query->head->from, query->head->contact[j], NULL );
    notify_delete(__notify, query->head->from, NULL, NULL );
    notify_change(__notify, query->head->to, 0, &cmd_submit_notify, foo);
    return;
}

/**
 * XXX
 */
static void cmd_maintain_subscribe(Sip_Service *foo, const Sip__Query *query, Sip__Answer_Closure closure,
        void *opaque) {

    if (!query || !query->head || !query->request) {

        closure(NULL, opaque);
        return;
    }
    log_debug("%s: %p %s", __PRETTY_FUNCTION__, query->request, query->head->version);

    Sip__Answer result[] = { SIP__ANSWER__INIT };
    Sip__Head head[] = { SIP__HEAD__INIT };

    result->response = SIP_OK;
    result->head = head;
    result->head->version = query->head->version;
    result->head->via = query->head->via;
    result->head->n_via = query->head->n_via;
    result->head->from = query->head->from;
    result->head->to = query->head->to;
    result->head->call_id = query->head->call_id;
    result->head->cseq = query->head->cseq;
    result->head->record_route = query->head->record_route;
    result->head->n_record_route = query->head->n_record_route;

    closure(result, opaque);

    int n = query->head->n_other;
    while (n--) {
        if (!strcasecmp("Event", query->head->other[n]->name ? : "")) {
            if (!strcmp("presence", query->head->other[n]->value ? : "")) {
                notify_update(__notify, query->head->from, query->head->contact[0], query->head->to,
                        query->head->call_id, -1, &cmd_submit_notify, foo);
            }
            break;
        }
    }
}

#else
#define cmd_maintain_registrate NULL
#define cmd_maintain_subscribe  NULL
#endif
#define cmd_maintain_invite     cmd_maintain_request
#define cmd_maintain_option     cmd_maintain_request
#define cmd_maintain_cancel     cmd_maintain_request
#define cmd_maintain_bye        cmd_maintain_request
#define cmd_maintain_info       cmd_maintain_request
#define cmd_maintain_notify     cmd_maintain_request

/**
 *
 */
int cmd_finalize(const osip_message_t *m, const Sip__Answer_Closure closure, void *opaque) {
    log_debug("%s: %p %p %p", __PRETTY_FUNCTION__, m, closure, opaque);

    if (closure) {
        Sip__Answer *result = sip__answer__proto(m);
        if (result) {
            closure(result, opaque);
            sip__answer__free_unpacked(result, 0);

            return 0;
        }
        closure(NULL, opaque);
    }
    return -1;
}

/**
 *
 */
static void cmd_maintain_request(Sip_Service *, const Sip__Query *query, const Sip__Answer_Closure closure,
                                 void *opaque) {
    if (!query || !query->head || !query->request || !query->head->cseq) {
        closure(NULL, opaque);
        return;
    }
    log_debug("%s: %s %p %p", __PRETTY_FUNCTION__, query->head->cseq->method, closure, opaque);

    osip_message_t *m = sip__query__unproto(query, query->head->cseq->method, FULL_REQUEST_BITSET);
    if (!m) {
        closure(NULL, opaque);
        return;
    }
    sip_proxy(m, closure, opaque);
}

#define cmd_finalise_registrate cmd_finalise_request
#define cmd_finalise_invite     cmd_finalise_request
#define cmd_finalise_option     cmd_finalise_request
#define cmd_finalise_cancel     cmd_finalise_request
#define cmd_finalise_bye        cmd_finalise_request
#define cmd_finalise_info       cmd_finalise_request
#define cmd_finalise_subscribe  cmd_finalise_request

struct finalise {
    int id;
};

/**
 *
 */
static void cmd_finalise_request(const Sip__Answer *result, void *opaque) {
    struct finalise *final = opaque;
    if (!result) {
        final->id = 0;
        return;
    }

    osip_message_t *m = sip__answer__unproto(result, FULL_RESPONSE_BITSET);
    if (m && m->status_code)
        return sip_finalize(m, final->id);

    sip_finalize_failure(final->id);
}

/**
 *
 */
int cmd_initiate_registrate(osip_transaction_t *a, osip_message_t *m) {
    if (!__cmd || !__cmd->ready)
        return -1;

    log_debug("%s: %p %s", __PRETTY_FUNCTION__, m->req_uri, m->sip_version);

    Sip__Query *query = sip__query__proto(m);
    if (!query)
        return -2;

    struct finalise final = {.id = a->transactionid};
    sip__registrate(__cmd->base, query, cmd_finalise_registrate, &final);
    sip__query__free_unpacked(query, 0);
    return final.id ? 0 : -1;
}

/**
 *
 */
int cmd_initiate_invite(osip_transaction_t *a, osip_message_t *m) {
    if (!__cmd || !__cmd->ready)
        return -1;

    log_debug("%s: %p %s", __PRETTY_FUNCTION__, m->req_uri, m->sip_version);

    Sip__Query *query = sip__query__proto(m);
    if (!query)
        return -2;

    struct finalise final = {.id = a->transactionid};
    sip__invite(__cmd->base, query, cmd_finalise_invite, &final);
    sip__query__free_unpacked(query, 0);
    return final.id ? 0 : -1;
}

/**
 *
 */
int cmd_initiate_option(osip_transaction_t *a, osip_message_t *m) {
    if (!__cmd || !__cmd->ready)
        return -1;

    log_debug("%s: %p %s", __PRETTY_FUNCTION__, m->req_uri, m->sip_version);

    Sip__Query *query = sip__query__proto(m);
    if (!query)
        return -2;

    struct finalise final = {.id = a->transactionid};
    sip__option(__cmd->base, query, cmd_finalise_option, &final);
    sip__query__free_unpacked(query, 0);
    return final.id ? 0 : -1;
}

/**
 *
 */
int cmd_initiate_cancel(osip_transaction_t *a, osip_message_t *m) {
    if (!__cmd || !__cmd->ready)
        return -1;

    log_debug("%s: %p %s", __PRETTY_FUNCTION__, m->req_uri, m->sip_version);

    Sip__Query *query = sip__query__proto(m);
    if (!query)
        return -2;

    struct finalise final = {.id = a->transactionid};
    sip__cancel(__cmd->base, query, cmd_finalise_cancel, &final);
    sip__query__free_unpacked(query, 0);
    return final.id ? 0 : -1;
}

/**
 *
 */
int cmd_initiate_bye(osip_transaction_t *a, osip_message_t *m) {
    if (!__cmd || !__cmd->ready)
        return -1;

    log_debug("%s: %p %s", __PRETTY_FUNCTION__, m->req_uri, m->sip_version);

    Sip__Query *query = sip__query__proto(m);
    if (!query)
        return -2;

    struct finalise final = {.id = a->transactionid};
    sip__bye(__cmd->base, query, cmd_finalise_bye, &final);
    sip__query__free_unpacked(query, 0);
    return final.id ? 0 : -1;
}

/**
 *
 */
int cmd_initiate_info(osip_transaction_t *a, osip_message_t *m) {
    if (!__cmd || !__cmd->ready)
        return -1;

    log_debug("%s: %p %s", __PRETTY_FUNCTION__, m->req_uri, m->sip_version);

    Sip__Query *query = sip__query__proto(m);
    if (!query)
        return -2;

    struct finalise final = {.id = a->transactionid};
    sip__info(__cmd->base, query, cmd_finalise_info, &final);
    sip__query__free_unpacked(query, 0);
    return final.id ? 0 : -1;
}

/**
 *
 */
int cmd_initiate_subscribe(osip_transaction_t *a, osip_message_t *m) {
    if (!__cmd || !__cmd->ready)
        return -1;

    log_debug("%s: %p %s", __PRETTY_FUNCTION__, m->req_uri, m->sip_version);

    Sip__Query *query = sip__query__proto(m);
    if (!query)
        return -2;

    struct finalise final = {.id = a->transactionid};
    sip__subscribe(__cmd->base, query, cmd_finalise_subscribe, &final);
    sip__query__free_unpacked(query, 0);
    return final.id ? 0 : -1;
}

/**
 *
 */
static int cmd_error(const long err, const char *msg, void *opaque) {
    struct net_event *net = opaque;
    if (!net)
        return -1;

    log_error("%s: %i/%s/%s [%s]",
              __PRETTY_FUNCTION__, net->event->ev_fd, net->net->name, net->app->name, msg);

    net->err = err;
    return -2;
}

/**
 *
 */
static int cmd_relay(char *buf, const unsigned short len, void *opaque) {
    struct net_event *net = opaque;
    if (!net)
        return -1;

    log_debug("%s: %i/%s/%s",
              __PRETTY_FUNCTION__, net->event->ev_fd, net->net->name, net->app->name);

    mem_t *mem = mem_new(buf, len, 0, 0);
    if (!mem)
        return -1;
    map_push_back(net->outgoing, mem);
    if (!__con->id)
        return net_send(net, mem);
    net_event_update(net, NULL);
    return 0;
}

/**
 *
 */
static int cmd_acquire(net_event_t *net) {
    log_debug("%s: %i/%s/%s [%s]", __PRETTY_FUNCTION__, net->event->ev_fd, net->net->name, net->app->name,
              net_event_type(net));

    if (net_is_connected(net)) {
        net_event_close(__con);
        __con = net;
        __cmd->peer->host = __con->peer->host;
        __cmd->peer->port = __con->peer->port;
        __cmd->subscribes = 0;
        __cmd->ready = 0;
        const unsigned short port = net_event_get_port(net);
        return __cmd->begin(__cmd, port, net);
    }

    return 0;
}


/**
 *
 */
static int cmd_execute(net_event_t *net) {
    log_debug("%s: %i/%s/%s [%hu:%hu:%hu]", __PRETTY_FUNCTION__, net->event->ev_fd, net->net->name, net->app->name,
              net->incoming->begin,
              net->incoming->done,
              net->incoming->end);

    const int len = __cmd->command(__cmd,
                                   net->incoming->begin + net->incoming->buffer,
                                   net->incoming->end - net->incoming->begin,
                                   net);
    if (len < 0)
        return -1;

    net->incoming->begin += len;
    net->incoming->done = net->incoming->begin;

    return 0;
}

/**
 *
 * @param
 * @param
 * @param 
 */
static void cmd_oneshot(int, short, void *) {
    net_open(&__g_net_TCP, &__g_app_CMD, __cmd->peer->host, __cmd->peer->port);
}

/**
 * 
 * @param net
 * @return 
 */
static int cmd_release(net_event_t *net) {
    log_debug("%s: %i/%s/%s [%lli]", __PRETTY_FUNCTION__, net->event->ev_fd, net->net->name, net->app->name, net->id);

    const struct timeval tv = {1, 0};
    if (__con == net) {
        if (!net->id)
            __cmd->end(__cmd, net);
        else
            net_event_oneshot(cmd_oneshot, NULL, &tv);
        __con = NULL;
    }

    return 0;
}

/**
 *
 * @param net
 * @return
 */
static int cmd_timeout(net_event_t *net) {
    if (!__cmd || !__cmd->ready)
        return -1;

    log_debug("%s: %i/%s/%s [%lli]", __PRETTY_FUNCTION__, net->event->ev_fd, net->net->name, net->app->name, net->id);

    return __cmd->ping(__cmd, __con);
}

/**
 *
 */
int cmd_init() {
    Sip_Service sip = SIP__INIT(cmd_maintain_);
    sip__init(&sip, NULL);

    __cmd = transport_new(&sip.base,
                          "sip", NULL, NULL, hostname, MQTT_CONNECT_CLEAN_SESSION, 60,
                          cmd_error, cmd_relay, NULL);
    if (!__cmd)
        return -1;
    return 0;
}

/**
 *
 */
void cmd_free() {
    __cmd->destroy(__cmd);
}

static char hostname[256] = "FIXME";

const app_t __g_app_CMD = {
    "CMD",
    "0.0.0.0",
    hostname,
    "5059",
    cmd_acquire,
    cmd_release,
    cmd_execute,
    cmd_timeout,
    {{NULL, NULL}},
    {30, 0}
};
