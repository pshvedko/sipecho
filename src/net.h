/*
 * net.h
 *
 *  Created on: Sep 24, 2013
 *      Author: shved
 */

#ifndef NET_H_
#define NET_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <event.h>
#include <osipparser2/osip_uri.h>

#include "lib/common/map.h"

#include "mem.h"

typedef struct app app_t;

typedef struct net {
    const char *name;
    unsigned flag;
    int protocol;
    int family;
    int type;

    int (*init)(const struct net *);

    int (*free)(const struct net *);

    int (*open)(const struct net *, const char *, unsigned short);

    int (*read)(int, struct mem *, void *);

    int (*send)(int, struct mem *, void *);

    int (*block)(int, int);

    int (*connect)(const struct net *, const char *, unsigned short);

    int (*accept)(int, struct sockaddr *, socklen_t *);

    int (*assign)(const struct net *, int, int, void **);

    int (*dismiss)(void **);

    void *foo;
} net_t;

extern const net_t __g_net_TCP;
extern const net_t __g_net_UDP;
extern const net_t __g_net_TLS;

typedef struct net_event {
    long long id;
    const struct net *net;
    const struct app *app;
    struct mem *incoming;
    struct map *outgoing;
    struct map *entities;
    struct event *event;

    enum {
        NET_TYPE_UNDEFINED = 0,
        NET_TYPE_LISTEN,
        NET_TYPE_ACCEPT,
        NET_TYPE_CONNECT,
    } type;

    struct {
        const char *host;
        unsigned short port;
    } peer[1];

    long err;
    void *foo;
} net_event_t;

#define net_is_connected(net)	(((net_event_t *)(net))->type == NET_TYPE_CONNECT)
#define net_is_accepted(net)	(((net_event_t *)(net))->type == NET_TYPE_ACCEPT)
#define net_is_listen(net)		(((net_event_t *)(net))->type == NET_TYPE_LISTEN)

int net_init();

int net_loop();

int net_port(const osip_uri_t *);

int net_bind(const net_t *, const app_t *);

int net_open(const net_t *, const app_t *, const char *, unsigned short);

int net_send(net_event_t *, mem_t *);

net_event_t *net_event_connect(const char *, const char *, const char *, unsigned short);

net_event_t *net_event_open(const net_t *, const app_t *);

net_event_t *net_event_new(int, short, void (*)(int, short, void *), const net_t *net, const app_t *app, int);

net_event_t *net_event_find_by_id(long long);

int net_event_add(net_event_t *, const struct timeval *);

int net_event_oneshot(void (*)(int, short, void *), void *, const struct timeval *);

int net_event_timeout_end(void **);

int net_event_timeout_begin(void **, void (*)(int, short, void *), void *, const struct timeval *);

void net_event_update(net_event_t *, const struct timeval *);

int net_event_free(net_event_t *);

void net_event_close(net_event_t *);

void net_free();

net_event_t *net_event_find_by_bind(const char *, const char *);

const char *net_event_type(net_event_t *);

#endif /* NET_H_ */
