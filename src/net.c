/*
 * net.c
 *
 *  Created on: Sep 26, 2013
 *      Author: shved
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include "log.h"
#include "net.h"

#include <stdbool.h>

#include "app.h"

static struct event_base *__net = NULL;

static struct event __sig_INT, __sig_TERM, __sig_USR1, __sig_USR2;

static struct map *__event = NULL;

static long long __id = 0;

/**
 *
 */
int net_port2(const char *scheme, const char *port) {
    if (port)
        return atoi(port);
    else if (!scheme)
        return 0;
    else if (strcasecmp("SIP", scheme) != 0)
        return 5060;
    else if (strcasecmp("SIPS", scheme) != 0)
        return 5061;
    else
        return 0;
}

/**
 *
 */
int net_port(const osip_uri_t *u) {
    if (!u)
        return 0;
    else
        return net_port2(u->scheme, u->port);
}

/**
 *
 */
int net_event_oneshot(const event_callback_fn callback, void *arg, const struct timeval *timeout) {
    return event_base_once(__net, -1, EV_TIMEOUT, callback, arg, timeout);
}

/**
 *
 */
int net_event_timeout_begin(void **e, const event_callback_fn callback, void *arg,
                            const struct timeval *timeout) {
    if (e) {
        if (e[0])
            event_del(e[0]);
        else
            e[0] = event_new(__net, -1, EV_TIMEOUT, callback, arg);
        if (e[0]) {
            return event_add(e[0], timeout);
        }
    }
    return -1;
}

/**
 *
 */
int net_event_timeout_end(void **e) {
    if (!e)
        return -1;

    if (e[0])
        event_free(e[0]);

    e[0] = 0;

    return 0;
}

/**
 *
 */
static int net_event_delete(net_event_t *event) {
    if (!event)
        return -1;

    log_debug("%s: %i/%s", __PRETTY_FUNCTION__, event->event->ev_fd, event->net->name);

    if (event->app->release)
        event->app->release(event);

    if (event->net->dismiss)
        event->net->dismiss(&event->foo);

    map_free(event->entities);
    map_free(event->outgoing);
    mem_free(event->incoming);

    const int s = event_get_fd(event->event);

    event_del(event->event);
    event_free(event->event);

    switch (event->type) {
        case NET_TYPE_LISTEN:
            if (event->net->free)
                event->net->free(event->net);
        default:
            break;
    }
    free(event);

    return s;
}

/**
 *
 * @param event
 * @return
 */
static void net_event_delete_id(net_event_t *event) {
    if (!event)
        return;

    event->id = 0;

    const int s = net_event_delete(event);
    if (s > 0) {
        close(s);
    }
}

/**
 *
 */
int net_event_free(net_event_t *event) {
    if (event)
        map_get(__event, event, 0);

    return net_event_delete(event);
}

/**
 *
 */
void net_event_close(net_event_t *event) {
    if (event) {
        const int s = net_event_free(event);
        if (s > 0) {
            close(s);
        }
    }
}

/**
 *
 */
static void net_event_fail(net_event_t *event, const char *name) {
    int err_num = 0;
    socklen_t err_len = sizeof(err_num);

    const int s = event_get_fd(event->event);
    if (s != -1) {
        const int n = getsockopt(s, SOL_SOCKET, SO_ERROR, &err_num, &err_len);
        if (n != -1)
            log_error("%s: %i/%s/%s: %s", name, s, event->net->name, event->app->name, strerror(err_num));
        else
            log_error("%s: %i/%s/%s: Failed", name, s, event->net->name, event->app->name);
    }
    return net_event_close(event);
}

/**
 *
 */
static void net_callback_inout(const int s, const short flag, void *arg) {
    net_event_t *event = arg;

    log_debug("%s: %i/%s/%s [%hu]", __PRETTY_FUNCTION__, s, event->net->name, event->app->name, flag);

    if (flag) {
        do {
            if (flag & EV_READ) {
                const int n = event->net->read(s, event->incoming, event->foo);
                if (n == -1)
                    break; /* shit happens */
                if (n == 0 && event->net->protocol != IPPROTO_UDP)
                    break; /* close */
                while (!mem_empty(event->incoming) && !event->app->execute(event)) {
                    mem_align(event->incoming);
                }
            }
            if (event->err)
                break;
            if (flag & EV_WRITE) {
                mem_t *out = map_front(event->outgoing);
                if (out) {
                    const int n = event->net->send(s, out, event->foo);
                    if (n == -1)
                        break; /* shit happens */
                    if (n == 0) {
                        map_pop(event->outgoing);
                        mem_free(out);
                    }
                }
            }
            return net_event_update(event, NULL);
        } while (0);
    }

    if (event->type == NET_TYPE_LISTEN) {
        const net_t *net = event->net;
        const app_t *app = event->app;

        net_event_fail(event, __PRETTY_FUNCTION__);

        event = net_event_open(net, app);
        if (event)
            event_add(event->event, NULL);
    } else
        net_event_close(event);
}

/**
 *
 */
static void net_callback_accept(const int s, const short flag, void *arg) {
    net_event_t *event = arg;

    if (!event->net->accept)
        return net_callback_inout(s, flag, arg);

    struct sockaddr a;
    socklen_t l = sizeof(a);

    if (flag & EV_READ) {
        const int c = event->net->accept(s, &a, &l);
        if (c != -1) {
            net_event_t *child = net_event_new(c, EV_READ, net_callback_inout, event->net, event->app,
                                               NET_TYPE_ACCEPT);
            if (child) {
                if (event->app->map->destroy) {
                    child->entities = map_new(event->app->map->destroy, event->app->map->compare, NULL);
                }
                child->outgoing = map_new(mem_delete, NULL);
                child->incoming = mem_new(NULL, 4096 << 2, &a, l);

                log_debug("%s: %i/%s/%s", __PRETTY_FUNCTION__, c, child->net->name, child->app->name);

                if ((child->app->acquire && child->app->acquire(child)) ||
                    (child->net->assign &&
                     child->net->assign(child->net, child->event->ev_fd, child->type, &child->foo))) {
                    log_error("%s: %i/%s/%s %m", __PRETTY_FUNCTION__, c, child->net->name, child->app->name);
                    net_event_close(child);
                } else
                    event_add(child->event, NULL);
            } else
                close(c);
        }
    }

    if (flag & EV_TIMEOUT)
        if (event->app->timeout)
            event->app->timeout(event);

    event_add(event->event, event->app->timeout ? &event->app->delay : NULL);
}

/**
 *
 */
static void net_callback_connect(const int s, const short flag, void *arg) {
    net_event_t *event = arg;

    if (flag & EV_WRITE) {
        const int n = getpeername(s, &event->incoming->peer->a, &event->incoming->peer->l);
        if (n != -1) {
            log_debug("%s: %i/%s/%s", __PRETTY_FUNCTION__, s, event->net->name, event->app->name);
            event_assign(event->event, __net, s, EV_READ | EV_WRITE, net_callback_inout, event);
            net_event_update(event, NULL);
            return;
        }
    }
    net_event_fail(event, __PRETTY_FUNCTION__);
}

/**
 * *
 */
net_event_t *net_event_connect(const char *net_name, const char *app_name, const char *host,
                               const unsigned short port) {
    net_event_t *bound = net_event_find_by_bind(net_name, app_name);
    if (bound) {
        if (!bound->net->connect) {
            return bound;
        }
        const int s = bound->net->connect(bound->net, host, port);
        if (s != -1) {
            net_event_t *event = net_event_new(s, EV_WRITE, net_callback_connect,
                                               bound->net, bound->app,
                                               NET_TYPE_CONNECT);
            if (event) {
                event->peer->host = host;
                event->peer->port = port;

                if (event->app->map->destroy) {
                    event->entities = map_new(event->app->map->destroy, event->app->map->compare, NULL);
                }
                event->outgoing = map_new(mem_delete, NULL);
                event->incoming = mem_new(NULL, 4096, NULL, 0);

                log_debug("%s: %i/%s/%s %s:%hu", __PRETTY_FUNCTION__, s, event->net->name, event->app->name,
                          host, port);

                if ((event->app->acquire && event->app->acquire(event)) ||
                    (event->net->assign &&
                     event->net->assign(event->net, event->event->ev_fd, event->type, &event->foo))) {
                    log_error("%s: %i/%s/%s %m", __PRETTY_FUNCTION__, s, event->net->name, event->app->name);
                    net_event_close(event);
                } else
                    return event;
            }
        }
    }
    return NULL;
}

/**
 *
 */
net_event_t *net_event_open(const net_t *net, const app_t *app) {
    const int s = net->open(net, app->host, net_port2(app->name, app->port));
    if (s != -1) {
        net_event_t *event = net_event_new(s, EV_READ, net_callback_accept, net, app, NET_TYPE_LISTEN);
        if (event) {
            if (event->app->map->destroy) {
                event->entities = map_new(event->app->map->destroy, event->app->map->compare, NULL);
            }
            event->outgoing = map_new(mem_delete, NULL);
            event->incoming = mem_new(NULL, 4096, NULL, 0);

            log_debug("%s: %i/%s", __PRETTY_FUNCTION__, s, event->net->name);

            if ((event->net->init && event->net->init(net)) ||
                (event->app->acquire && event->app->acquire(event)) ||
                (event->net->assign && event->net->assign(event->net, event->event->ev_fd, event->type, &event->foo))) {
                log_error("%s: %i/%s/%s %m", __PRETTY_FUNCTION__, s, event->net->name, event->app->name);
                net_event_close(event);
            } else
                return event;
        }
    }
    return NULL;
}

/**
 *
 * @param net
 * @param mem
 * @return
 */
int net_send(net_event_t *net, mem_t *mem) {
    if (net->net->block)
        if (net->net->block(net->event->ev_fd, 0) < 0)
            return -1;
    if (net->net->send(net->event->ev_fd, mem, net->foo) < 0)
        return -1;
    return 0;
}

/**
 *
 */
int net_bind(const net_t *net, const app_t *app) {
    net_event_t *event = net_event_open(net, app);
    if (!event) {
        log_fatal("%s: %s/%s/%s:%s failed: %m",
                  __PRETTY_FUNCTION__, net->name, app->name, app->host, app->port);
        return -1;
    }
    return event_add(event->event, event->app->timeout ? &event->app->delay : NULL);
}

/**
 *
 */
int net_open(const net_t *net, const app_t *app, const char *host, const unsigned short port) {
    net_event_t *event = net_event_connect(net->name, app->name, host, port);
    if (!event) {
        log_fatal("%s: %s/%s/%s:%hu failed: %m", __PRETTY_FUNCTION__, net->name, app->name, host, port);
        return -1;
    }
    return event_add(event->event, NULL);
}

/**
 *
 */
net_event_t *net_event_new(const int s, const short flag, void (*call)(int, short, void *), const net_t *net,
                           const app_t *app, const int type) {
    net_event_t *event = calloc(1, sizeof(net_event_t));
    if (event) {
        event->event = event_new(__net, s, flag, call, event);
        if (!event->event) {
            net_event_delete(event);
            return NULL;
        }
        event->net = net;
        event->app = app;
        event->id = ++__id;
        event->type = type;

        assert(!map_push(__event, event));
    }
    return event;
}

/**
 *
 */
int net_event_add(net_event_t *event, const struct timeval *timeout) {
    if (event)
        return event_add(event->event, timeout);
    return -1;
}

/**
 *
 */
void net_event_update(net_event_t *event, const struct timeval *timeout) {
    if (!event)
        return;

    const int s = event_get_fd(event->event);
    if (s == -1)
        return;
    else
        event_del(event->event);

    void (*call)(int, short, void *) = event_get_callback(event->event);
    if (call) {
        const short flag = event_get_events(event->event);
        if (flag) {
            if (map_size(event->outgoing))
                event_assign(event->event, __net, s, flag | EV_WRITE, call, event);
            else if (event->type == NET_TYPE_CONNECT && event->entities != NULL
                     && map_size(event->entities) == 0) {
                net_event_free(event);
                return;
            } else
                event_assign(event->event, __net, s, flag & ~EV_WRITE, call, event);
        }
    }
    event_add(event->event, timeout);
}

/**
 *
 */
static void net_callback_signal(const int s, short flag, void *arg) {
    log_alert("%s: %i", __PRETTY_FUNCTION__, s);

    switch (s) {
        case SIGUSR1:
            log_increase();
            break;
        case SIGUSR2:
            log_decrease();
            break;
        case SIGINT:
        case SIGTERM:
            event_base_loopbreak(__net);
            break;
        default:
            break;
    }
}

/**
 *
 */
net_event_t *net_event_find_by_id(const long long id) {
    net_event_t key[1] = {{.id = id}};

    return map_find(__event, key, 1);
}

/**
 *
 */
net_event_t *net_event_find_by_bind(const char *net_name, const char *app_name) {
    net_t net[1] = {{.name = net_name}};

    app_t app[1] = {{.name = app_name}};

    net_event_t key[1] = {{.net = net, .app = app, .type = NET_TYPE_LISTEN,},};

    return map_find(__event, key, 2);
}

/**
 *
 */
static int net_event_cmp(net_event_t *a, net_event_t *b, const int i) {
    switch (i) {
        case 0:
            return memcmp(&a, &b, sizeof(a));
        case 1:
            return memcmp(&a->id, &b->id, sizeof(a->id));
        case 2: {
            int cmp = strcasecmp(a->net->name, b->net->name);
            if (cmp == 0)
                cmp = strcasecmp(a->app->name, b->app->name);
            if (cmp == 0)
                cmp = memcmp(&a->type, &b->type, sizeof(a->type));
            if (cmp == 0 && a->type != NET_TYPE_LISTEN)
                cmp = memcmp(&a->event, &b->event, sizeof(a->event));
            return cmp;
        }
        default:
            return -1;
    }
}

const char *net_event_type(net_event_t *event) {
    static char *const types[] = {
        "UNDEFINED",
        "LISTEN",
        "ACCEPT",
        "CONNECT",
    };
    return types[event->type & NET_TYPE_CONNECT];
}

/**
 *
 */
int net_init() {
    __net = event_base_new();
    if (!__net)
        return -1;

    __event = map_new(
        (map_del_t) &net_event_delete_id,
        (map_cmp_t) &net_event_cmp,
        (map_cmp_t) &net_event_cmp,
        (map_cmp_t) &net_event_cmp, NULL);
    if (!__event)
        return -1;

    evsignal_assign(&__sig_INT, __net, SIGINT, net_callback_signal, NULL);
    evsignal_add(&__sig_INT, NULL);
    evsignal_assign(&__sig_TERM, __net, SIGTERM, net_callback_signal, NULL);
    evsignal_add(&__sig_TERM, NULL);
    evsignal_assign(&__sig_USR1, __net, SIGUSR1, net_callback_signal, NULL);
    evsignal_add(&__sig_USR1, NULL);
    evsignal_assign(&__sig_USR2, __net, SIGUSR2, net_callback_signal, NULL);
    evsignal_add(&__sig_USR2, NULL);

    return 0;
}

/**
 *
 */
void net_free() {
    evsignal_del(&__sig_INT);
    evsignal_del(&__sig_TERM);
    evsignal_del(&__sig_USR1);
    evsignal_del(&__sig_USR2);

    map_free(__event);

    event_base_free(__net);
}

/**
 *
 */
int net_loop() {
    return event_base_loop(__net, 0);
}
