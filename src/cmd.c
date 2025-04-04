/*
 * acp.c
 *
 *  Created on: Oct 7, 2013
 *      Author: shved
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <uuid/uuid.h>

#include "lib/common/map.h"
#include "lib/proto/sip/sip.h"
#include "lib/proto/transport.h"

#include "app.h"
#include "log.h"

typedef struct session {
    int id;
    unsigned char uuid[16];
} session_t;

static struct map *__ses; /**< Transaction id, map of @session_t */

static net_event_t *__con = NULL;

static char hostname[];

#define cmd_maintain_registration   cmd_maintain_request
#define cmd_maintain_subscribe      cmd_maintain_request
#define cmd_maintain_invite         cmd_maintain_request
#define cmd_maintain_option         cmd_maintain_request
#define cmd_maintain_cancel         cmd_maintain_request
#define cmd_maintain_bye            cmd_maintain_request
#define cmd_maintain_info           cmd_maintain_request
#define cmd_maintain_notify         cmd_maintain_request

/**
 *
 * @param m
 * @param closure
 * @param opaque
 * @param id
 * @return
 */
int cmd_finalize(const osip_message_t *m, const Sip__Message_Closure closure, void *opaque, const uuid_t id) {
    log_debug("%s: %p %p %p", __PRETTY_FUNCTION__, m, closure, opaque);

    if (closure) {
        Sip__Message *result = sip__message__proto(m, id);
        if (result) {
            closure(result, opaque);
            sip__message__free_unpacked(result, NULL);
            return 0;
        }
        closure(NULL, opaque);
    }
    return -1;
}

static void cmd_finalise_request(const Sip__Message *reply, void *opaque) {
    if (!reply) {
        int *ret = opaque;
        if (ret)
            *ret = -1;
        return;
    }

    log_debug("%s: %p %p", __PRETTY_FUNCTION__, reply, opaque);

    uuid_t id = {};
    osip_message_t *m = sip__message__unproto(reply, FULL_RESPONSE_BITSET, id);
    if (uuid_is_null(id))
        osip_message_free(m);
    else if (m && m->status_code)
        sip_finalize(m, id);
    else
        sip_finalize_failure(id);
}

static void cmd_maintain_request(Sip_Service *, const Sip__Message *query, const Sip__Message_Closure closure,
                                 void *opaque) {
    if (!opaque)
        return;

    if (query && query->response)
        cmd_finalise_request(query, NULL);

    if (!query || query->response || !query->head || !query->request || !query->head->cseq) {
        closure(NULL, opaque);
        return;
    }

    log_debug("%s: %s %p %p", __PRETTY_FUNCTION__, query->head->cseq->method, closure, opaque);

    uuid_t id = {};
    osip_message_t *m = sip__message__unproto(query, FULL_REQUEST_BITSET, id);

    if (!m || uuid_is_null(id)) {
        closure(NULL, opaque);
        osip_message_free(m);
        return;
    }
    sip_proxy(m, closure, opaque, id);
}

#define cmd_finalise_registration   cmd_finalise_request
#define cmd_finalise_invite         cmd_finalise_request
#define cmd_finalise_option         cmd_finalise_request
#define cmd_finalise_cancel         cmd_finalise_request
#define cmd_finalise_bye            cmd_finalise_request
#define cmd_finalise_info           cmd_finalise_request
#define cmd_finalise_subscribe      cmd_finalise_request

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

static void cmd_session_delete(session_t *ses) {
    if (ses)
        free(ses);
}

static int cmd_session_cmp(const session_t *a, const session_t *b, const int i) {
    switch (i) {
        case 0:
            return map_mem4_cmp(&a->id, &b->id, i);
        case 1:
            return uuid_compare(a->uuid, b->uuid);
        default:
            return -1;
    }
}

static const unsigned char *cmd_session_get(const int id) {
    struct session key = {.id = id, .uuid = {}}, *ses = map_find(__ses, &key, 0);
    if (!ses) {
        ses = calloc(1, sizeof(struct session));
        ses->id = id;
        uuid_generate_random(ses->uuid);
        if (map_push(__ses, ses)) {
            cmd_session_delete(ses);
            return NULL;
        }
    }
    return ses->uuid;
}

/**
 * 
 * @param id 
 * @param in 
 * @return 
 */
int cmd_session_add(const int id, const uuid_t in) {
    struct session *ses = calloc(1, sizeof(struct session));
    if (!ses)
        return -1;
    ses->id = id;
    uuid_copy(ses->uuid, in);
    return map_push(__ses, ses);
}

/**
 * 
 * @param in 
 * @return 
 */
int cmd_session_find(const uuid_t in) {
    struct session key = {.id = 0, .uuid = UUID_COPY(in)}, *ses = map_find(__ses, &key, 1);
    if (ses) {
        ses->uuid[0] = in[0];
        return ses->id;
    }
    return -1;
}

/**
 * 
 * @param id 
 * @param out 
 * @return 
 */
int cmd_session_destroy(const int id, uuid_t out) {
    struct session key = {.id = id}, *ses = map_get(__ses, &key, 0);
    if (!ses)
        return -1;
    uuid_copy(out, ses->uuid);
    cmd_session_delete(ses);
    return 0;
}

/**
 * 
 * @param a 
 * @param m 
 * @return 
 */
int cmd_initiate_registration(osip_transaction_t *a, osip_message_t *m) {
    if (!__con)
        return -1;
    if (!cmd_ready(__con->foo))
        return -1;

    log_debug("%s: %p %s", __PRETTY_FUNCTION__, m->req_uri, m->sip_version);

    Sip__Message *query = sip__message__proto(m, cmd_session_get(a->transactionid));
    if (!query)
        return -2;

    int ret = 0;
    sip__registration(__con->foo, query, cmd_finalise_registration, &ret);
    sip__message__free_unpacked(query, NULL);
    return ret;
}

/**
 * 
 * @param a 
 * @param m 
 * @return 
 */
int cmd_initiate_invite(osip_transaction_t *a, osip_message_t *m) {
    if (!__con)
        return -1;
    if (!cmd_ready(__con->foo))
        return -1;

    log_debug("%s: %p %s", __PRETTY_FUNCTION__, m->req_uri, m->sip_version);

    Sip__Message *query = sip__message__proto(m, cmd_session_get(a->transactionid));
    if (!query)
        return -2;

    int ret = 0;
    sip__invite(__con->foo, query, cmd_finalise_invite, &ret);
    sip__message__free_unpacked(query, NULL);
    return ret;
}

/**
 * 
 * @param a 
 * @param m 
 * @return 
 */
int cmd_initiate_option(osip_transaction_t *a, osip_message_t *m) {
    if (!__con)
        return -1;
    if (!cmd_ready(__con->foo))
        return -1;

    log_debug("%s: %p %s", __PRETTY_FUNCTION__, m->req_uri, m->sip_version);

    Sip__Message *query = sip__message__proto(m, cmd_session_get(a->transactionid));
    if (!query)
        return -2;

    int ret = 0;
    sip__option(__con->foo, query, cmd_finalise_option, &ret);
    sip__message__free_unpacked(query, NULL);
    return ret;
}

/**
 * 
 * @param a 
 * @param m 
 * @return 
 */
int cmd_initiate_cancel(osip_transaction_t *a, osip_message_t *m) {
    if (!__con)
        return -1;
    if (!cmd_ready(__con->foo))
        return -1;

    log_debug("%s: %p %s", __PRETTY_FUNCTION__, m->req_uri, m->sip_version);

    Sip__Message *query = sip__message__proto(m, cmd_session_get(a->transactionid));
    if (!query)
        return -2;

    int ret = 0;
    sip__cancel(__con->foo, query, cmd_finalise_cancel, &ret);
    sip__message__free_unpacked(query, NULL);
    return ret;
}

/**
 * 
 * @param a 
 * @param m 
 * @return 
 */
int cmd_initiate_bye(osip_transaction_t *a, osip_message_t *m) {
    if (!__con)
        return -1;
    if (!cmd_ready(__con->foo))
        return -1;

    log_debug("%s: %p %s", __PRETTY_FUNCTION__, m->req_uri, m->sip_version);

    Sip__Message *query = sip__message__proto(m, cmd_session_get(a->transactionid));
    if (!query)
        return -2;

    int ret = 0;
    sip__bye(__con->foo, query, cmd_finalise_bye, &ret);
    sip__message__free_unpacked(query, NULL);
    return ret;
}

/**
 * 
 * @param a 
 * @param m 
 * @return 
 */
int cmd_initiate_info(osip_transaction_t *a, osip_message_t *m) {
    if (!__con)
        return -1;
    if (!cmd_ready(__con->foo))
        return -1;

    log_debug("%s: %p %s", __PRETTY_FUNCTION__, m->req_uri, m->sip_version);

    Sip__Message *query = sip__message__proto(m, cmd_session_get(a->transactionid));
    if (!query)
        return -2;

    int ret = 0;
    sip__info(__con->foo, query, cmd_finalise_info, &ret);
    sip__message__free_unpacked(query, NULL);
    return ret;
}

/**
 * 
 * @param a 
 * @param m 
 * @return 
 */
int cmd_initiate_subscribe(osip_transaction_t *a, osip_message_t *m) {
    if (!__con)
        return -1;
    if (!cmd_ready(__con->foo))
        return -1;

    log_debug("%s: %p %s", __PRETTY_FUNCTION__, m->req_uri, m->sip_version);

    Sip__Message *query = sip__message__proto(m, cmd_session_get(a->transactionid));
    if (!query)
        return -2;

    int ret = 0;
    sip__subscribe(__con->foo, query, cmd_finalise_subscribe, &ret);
    sip__message__free_unpacked(query, NULL);
    return ret;
}

/**
 * 
 * @param foo 
 * @param err 
 * @param msg 
 * @return 
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
 * @param foo 
 * @param buf 
 * @param len 
 * @return 
 */
static int cmd_relay(void *foo, char *buf, const unsigned short len) {
    net_event_t *net = foo;

    if (!net)
        return -1;

    log_debug("%s: %i/%s/%s",
              __PRETTY_FUNCTION__, net->event->ev_fd, net->net->name, net->app->name);

    // log_dump(buf, len);

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
    return transport_new(&sip.base, net->argv[0], net->argv[1], net->argv[2],
                         hostname, MQTT_CONNECT_CLEAN_SESSION, 60,
                         cmd_error, cmd_relay, net);
}

/**
 * 
 * @param net 
 * @return 
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
 * @param net 
 * @return 
 */
static int cmd_execute(net_event_t *net) {
    log_debug("%s: %i/%s/%s [%hu:%hu:%hu]", __PRETTY_FUNCTION__, net->event->ev_fd, net->net->name, net->app->name,
              net->incoming->begin,
              net->incoming->done,
              net->incoming->end);

    // log_dump(net->incoming->begin + net->incoming->buffer, net->incoming->end - net->incoming->begin);

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
    return net_open(&__g_net_TCP, &__g_app_CMD, net->peer->host, net->peer->port,
                    net->argv[0], net->argv[1], net->argv[2]);
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
 * @param net
 * @param ap
 * @return
 */
static int cmd_operand(net_event_t *net, const va_list ap) {
    net->argv = calloc(3, sizeof(const char *));
    if (!net->argv)
        return -1;
    net->argv[0] = va_arg(ap, const char*);
    net->argv[1] = va_arg(ap, const char*);
    net->argv[2] = va_arg(ap, const char*);
    return 0;
}

/**
 * 
 * @return
 */
int cmd_init() {
    __ses = map_new((map_del_t) &cmd_session_delete, (map_cmp_t) &cmd_session_cmp, (map_cmp_t) &cmd_session_cmp, NULL);
    if (!__ses)
        return -1;

    return 0;
}

/**
 * 
 */
void cmd_free() {
    map_free(__ses);
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
    cmd_operand,
    {{NULL, NULL}},
    {30, 0}
};
