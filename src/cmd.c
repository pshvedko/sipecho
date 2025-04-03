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
#include "log.h"

static net_event_t *__con = NULL;

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
int cmd_finalize(const osip_message_t *m, const Sip__Message_Closure closure, void *opaque) {
    log_debug("%s: %p %p %p", __PRETTY_FUNCTION__, m, closure, opaque);

    if (closure) {
        Sip__Message *result = sip__message__proto(m, 0);
        if (result) {
            closure(result, opaque);
            sip__message__free_unpacked(result, 0);

            return 0;
        }
        closure(NULL, opaque);
    }
    return -1;
}

/**
 *
 */
static void cmd_maintain_request(Sip_Service *, const Sip__Message *query, const Sip__Message_Closure closure,
                                 void *opaque) {
    if (!query || !query->head || !query->request || !query->head->cseq) {
        closure(NULL, opaque);
        return;
    }
    log_debug("%s: %s %p %p", __PRETTY_FUNCTION__, query->head->cseq->method, closure, opaque);

    osip_message_t *m = sip__message__unproto(query, query->head->cseq->method, FULL_REQUEST_BITSET, NULL);
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

static int cmd_ping(transport_t *cmd) {
    return cmd->ping(cmd, NULL);
}

static int cmd_end(transport_t *cmd) {
    return cmd->destroy(cmd, cmd->end(cmd, NULL));
}

static int cmd_begin(transport_t *cmd, const unsigned short port) {
    return cmd->begin(cmd, port, NULL);
}

static int cmd_command(transport_t *cmd, char *buf, const unsigned short len) {
    return cmd->command(cmd, buf, len, NULL);
}

static int cmd_destroy(transport_t *cmd) {
    return cmd->destroy(cmd, 0);
}

static int cmd_ready(transport_t *cmd) {
    if (cmd && cmd->ready)
        return 1;
    return 0;
}

/**
 *
 */
static void cmd_finalise_request(const Sip__Message *result, void *opaque) {
    if (!result) {
        int *ret = opaque;
        if (ret)
            *ret = -3;
        return;
    }

    int id;
    osip_message_t *m = sip__message__unproto(result, NULL, FULL_RESPONSE_BITSET, &id);
    if (id && m && m->status_code)
        return sip_finalize(m, id);

    sip_finalize_failure(id);
}

/**
 *
 */
int cmd_initiate_registrate(osip_transaction_t *a, osip_message_t *m) {
    if (!__con)
        return -1;
    if (!cmd_ready(__con->foo))
        return -1;

    log_debug("%s: %p %s", __PRETTY_FUNCTION__, m->req_uri, m->sip_version);

    Sip__Message *query = sip__message__proto(m, a->transactionid);
    if (!query)
        return -2;

    int ret = 0;
    sip__registrate(__con->foo, query, cmd_finalise_registrate, &ret);
    sip__message__free_unpacked(query, 0);
    return ret;
}

/**
 *
 */
int cmd_initiate_invite(osip_transaction_t *a, osip_message_t *m) {
    if (!__con)
        return -1;
    if (!cmd_ready(__con->foo))
        return -1;

    log_debug("%s: %p %s", __PRETTY_FUNCTION__, m->req_uri, m->sip_version);

    Sip__Message *query = sip__message__proto(m, a->transactionid);
    if (!query)
        return -2;

    int ret = 0;
    sip__invite(__con->foo, query, cmd_finalise_invite, &ret);
    sip__message__free_unpacked(query, 0);
    return ret;
}

/**
 *
 */
int cmd_initiate_option(osip_transaction_t *a, osip_message_t *m) {
    if (!__con)
        return -1;
    if (!cmd_ready(__con->foo))
        return -1;

    log_debug("%s: %p %s", __PRETTY_FUNCTION__, m->req_uri, m->sip_version);

    Sip__Message *query = sip__message__proto(m, a->transactionid);
    if (!query)
        return -2;

    int ret = 0;
    sip__option(__con->foo, query, cmd_finalise_option, &ret);
    sip__message__free_unpacked(query, 0);
    return ret;
}

/**
 *
 */
int cmd_initiate_cancel(osip_transaction_t *a, osip_message_t *m) {
    if (!__con)
        return -1;
    if (!cmd_ready(__con->foo))
        return -1;

    log_debug("%s: %p %s", __PRETTY_FUNCTION__, m->req_uri, m->sip_version);

    Sip__Message *query = sip__message__proto(m, a->transactionid);
    if (!query)
        return -2;

    int ret = 0;
    sip__cancel(__con->foo, query, cmd_finalise_cancel, &ret);
    sip__message__free_unpacked(query, 0);
    return ret;
}

/**
 *
 */
int cmd_initiate_bye(osip_transaction_t *a, osip_message_t *m) {
    if (!__con)
        return -1;
    if (!cmd_ready(__con->foo))
        return -1;

    log_debug("%s: %p %s", __PRETTY_FUNCTION__, m->req_uri, m->sip_version);

    Sip__Message *query = sip__message__proto(m, a->transactionid);
    if (!query)
        return -2;

    int ret = 0;
    sip__bye(__con->foo, query, cmd_finalise_bye, &ret);
    sip__message__free_unpacked(query, 0);
    return ret;
}

/**
 *
 */
int cmd_initiate_info(osip_transaction_t *a, osip_message_t *m) {
    if (!__con)
        return -1;
    if (!cmd_ready(__con->foo))
        return -1;

    log_debug("%s: %p %s", __PRETTY_FUNCTION__, m->req_uri, m->sip_version);

    Sip__Message *query = sip__message__proto(m, a->transactionid);
    if (!query)
        return -2;

    int ret = 0;
    sip__info(__con->foo, query, cmd_finalise_info, &ret);
    sip__message__free_unpacked(query, 0);
    return ret;
}

/**
 *
 */
int cmd_initiate_subscribe(osip_transaction_t *a, osip_message_t *m) {
    if (!__con)
        return -1;
    if (!cmd_ready(__con->foo))
        return -1;

    log_debug("%s: %p %s", __PRETTY_FUNCTION__, m->req_uri, m->sip_version);

    Sip__Message *query = sip__message__proto(m, a->transactionid);
    if (!query)
        return -2;

    int ret = 0;
    sip__subscribe(__con->foo, query, cmd_finalise_subscribe, &ret);
    sip__message__free_unpacked(query, 0);
    return ret;
}

/**
 *
 */
static int cmd_error(void *foo, const long err, const char *msg) {
    net_event_t *net = foo;
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
static int cmd_relay(void *foo, char *buf, const unsigned short len) {
    net_event_t *net = foo;

    if (!net)
        return -1;

    log_debug("%s: %i/%s/%s",
              __PRETTY_FUNCTION__, net->event->ev_fd, net->net->name, net->app->name);

    mem_t *mem = mem_new(buf, len, 0, 0);
    if (!mem)
        return -1;
    map_push_back(net->outgoing, mem);
    if (!net->id)
        return net_send(net, mem);
    net_event_update(net, NULL);
    return 0;
}

/**
 * 
 * @param net
 * @return 
 */
static void *cmd_new(net_event_t *net) {
    Sip_Service sip = SIP__INIT(cmd_maintain_);
    sip__init(&sip, NULL);
    return transport_new(&sip.base, "front", net->argv[0], net->argv[1],
                         hostname, MQTT_CONNECT_CLEAN_SESSION, 60,
                         cmd_error, cmd_relay, net);
}

/**
 *
 */
static int cmd_acquire(net_event_t *net) {
    log_debug("%s: %i/%s/%s [%s]", __PRETTY_FUNCTION__, net->event->ev_fd, net->net->name, net->app->name,
              net_event_type(net));

    if (net_is_connected(net)) {
        __con = net;
        __con->foo = cmd_new(__con);
        if (!__con->foo)
            return -1;
        return cmd_begin(net->foo, net_event_get_port(net));
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

    if (net->foo) {
        const int len = cmd_command(net->foo,
                                    net->incoming->begin + net->incoming->buffer,
                                    net->incoming->end - net->incoming->begin);
        if (len < 0)
            return -1;
        net->incoming->begin += len;
        net->incoming->done = net->incoming->begin;
    }
    return 0;
}

/**
 *
 * @param net
 * @return
 */
static int cmd_open(net_event_t *net) {
    if (net->foo)
        cmd_destroy(net->foo);
    return net_open(&__g_net_TCP, &__g_app_CMD, net->peer->host, net->peer->port, net->argv[0], net->argv[1]);
}

/**
 * 
 * @param net
 * @return 
 */
static int cmd_release(net_event_t *net) {
    log_debug("%s: %i/%s/%s [%lli]", __PRETTY_FUNCTION__, net->event->ev_fd, net->net->name, net->app->name, net->id);

    if (__con == net) {
        __con = NULL;
        if (net->id)
            return cmd_open(net);
        if (net->foo)
            return cmd_end(net->foo);
    }

    return 0;
}

/**
 *
 * @param net
 * @return
 */
static int cmd_timeout(net_event_t *net) {
    log_debug("%s: %i/%s/%s", __PRETTY_FUNCTION__, net->event->ev_fd, net->net->name, net->app->name);

    if (net->foo)
        return cmd_ping(net->foo);
    return 0;
}

/**
 *
 */
int cmd_init() { return 0; }

/**
 * 
 * @param net
 * @param ap
 * @return 
 */
static int cmd_connect(net_event_t *net, const va_list ap) {
    net->argv = calloc(2, sizeof(const char *));
    if (!net->argv)
        return -1;
    net->argv[0] = va_arg(ap, const char*);
    net->argv[1] = va_arg(ap, const char*);
    return 0;
}

/**
 *
 */
void cmd_free() { ; }

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
    cmd_connect,
    {{NULL, NULL}},
    {30, 0}
};
