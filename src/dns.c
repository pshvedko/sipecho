/*
 * dns.c
 *
 *  Created on: Nov 3, 2013
 *      Author: shved
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <arpa/inet.h>

#include <ldns/ldns.h>

#include "lib/common/map.h"

#include "app.h"
#include "log.h"

static int __id = 0;

static map_t *__dns = NULL;

static char hostname[];

typedef struct dns_entity {
    int id;

    void (*callback)(void *, const char *);

    void *opaque;
    char *host;
    struct timespec birth;
    struct sockaddr_storage *ss;
} dns_entity_t;

int dns_is_resolved(const char *host) {
    if (!host)
        return 0;

    union {
        struct sockaddr_in in4;
        struct sockaddr_in6 in6;
    } foo;

    if (1 == inet_pton(AF_INET, host, &foo.in4.sin_addr))
        return 1;

    if (1 == inet_pton(AF_INET6, host, &foo.in6.sin6_addr))
        return 1;

    return 0;
}

void dns_destroy2(dns_entity_t *dns) {
    if (dns->callback) {
        const char *result = NULL;
        if (dns->ss) {
            char address[INET6_ADDRSTRLEN];
            switch (dns->ss->ss_family) {
                case AF_INET:
                    result = inet_ntop(dns->ss->ss_family, &(((struct sockaddr_in *) dns->ss)->sin_addr),
                                       address, sizeof(address));
                    break;
                case AF_INET6:
                    result = inet_ntop(dns->ss->ss_family, &(((struct sockaddr_in6 *) dns->ss)->sin6_addr),
                                       address, sizeof(address));
                    break;
                default:
                    break;
            }
        }
        dns->callback(dns->opaque, result);
    }
    free(dns->ss);
    free(dns->host);
    free(dns);
}

void dns_destroy(void *item) {
    dns_destroy2(item);
}

int dns_compare2(const dns_entity_t *a, const dns_entity_t *b, int i) {
    return memcmp(&a->id, &b->id, sizeof(a->id));
}

int dns_compare(const void *a, const void *b, int i) {
    return dns_compare2(a, b, i);
}

void dns_delete(net_event_t *net) {
    ldns_resolver_deep_free(net->foo);
}

int dns_init() {
    __dns = map_new((map_del_t) &dns_delete, map_item_cmp, NULL);
    if (!__dns)
        return -1;

    return 0;
}

void dns_free() {
    map_free(__dns);
}

int dns_acquire(net_event_t *net) {
    log_debug("%s: %i/%s/%s", __PRETTY_FUNCTION__, net->event->ev_fd, net->net->name, net->app->name);

    net->foo = ldns_resolver_new();
    if (net->foo) {
        ldns_rdf *ip = ldns_rdf_new_frm_str(LDNS_RDF_TYPE_AAAA, net->app->hostname);
        if (ip == NULL)
            ip = ldns_rdf_new_frm_str(LDNS_RDF_TYPE_A, net->app->hostname);
        if (ip != NULL) {
            if (ldns_resolver_push_nameserver(net->foo, ip) == LDNS_STATUS_OK) {
                ldns_rdf_deep_free(ip);
                return map_push(__dns, net);
            }
            ldns_resolver_deep_free(net->foo);
            ldns_rdf_deep_free(ip);
        }
    }
    return -1;
}

int dns_release(net_event_t *net) {
    log_debug("%s: %i/%s/%s", __PRETTY_FUNCTION__, net->event->ev_fd, net->net->name, net->app->name);

    map_pure(__dns, net, 0);

    return 0;
}

int dns_execute(net_event_t *net) {
    log_debug("%s: %i/%s/%s", __PRETTY_FUNCTION__, net->event->ev_fd, net->net->name, net->app->name);

    ldns_pkt *dp = NULL;

    if (ldns_wire2pkt(&dp, (uint8_t *) net->incoming->buffer + net->incoming->done,
                      net->incoming->end - net->incoming->done) == LDNS_STATUS_OK) {
        ldns_rr_list *rr = ldns_pkt_get_section_clone(dp, LDNS_SECTION_ANSWER);
        if (rr) {
            dns_entity_t key[1] = {{.id = ldns_pkt_id(dp)}}, *dns = map_get(net->entities, key, 0);
            if (dns) {
                size_t i = ldns_rr_list_rr_count(rr);
                while (i--) {
                    const ldns_rr *a = ldns_rr_list_rr(rr, i);
                    if (LDNS_RR_TYPE_A == ldns_rr_get_type(a)) {
                        dns->ss = ldns_rdf2native_sockaddr_storage(ldns_rr_rdf(a, 0), -1, &i);
                        break;
                    }
                }
                dns_destroy(dns);
            }
            ldns_rr_list_deep_free(rr);
        }
        ldns_pkt_print(stdout, dp);
        ldns_pkt_free(dp);
    }
    net->incoming->begin = net->incoming->end;
    net->incoming->done = net->incoming->end;

    return -1;
}

mem_t *dns_query(ldns_resolver *r, const int id, const char *a, const ldns_rr_type t, const ldns_rr_class c,
                 const uint16_t f) {
    ldns_pkt *dp = NULL;
    mem_t *mem = NULL;
    if (!a)
        return NULL;
    if (ldns_resolver_nameserver_count(r) == 0)
        return NULL;
    if (ldns_resolver_tsig_keyname(r) && ldns_resolver_tsig_keydata(r))
        return NULL;

    ldns_rdf **ns = ldns_resolver_nameservers(r);
    if (!ns || !*ns)
        return NULL;

    size_t sl = 0;
    const uint16_t port = ldns_resolver_port(r);
    struct sockaddr sa;
    struct sockaddr_storage *ss = ldns_rdf2native_sockaddr_storage(ns[0], port, &sl);
    if (!ss)
        return NULL;

    memcpy(&sa, ss, sl);
    free(ss);

    ldns_rdf *n = ldns_dname_new_frm_str(a);
    if (!n)
        return NULL;

    if (ldns_rdf_get_type(n) == LDNS_RDF_TYPE_DNAME) {
        if (ldns_resolver_prepare_query_pkt(&dp, r, n, t ? t : LDNS_RR_TYPE_A, c ? c : LDNS_RR_CLASS_IN, f) ==
            LDNS_STATUS_OK) {
            ldns_pkt_set_id(dp, id);
            ldns_buffer *qb = ldns_buffer_new(4 * LDNS_MIN_BUFLEN);
            if (qb) {
                if (ldns_pkt2buffer_wire(qb, dp) == LDNS_STATUS_OK) {
                    const size_t l = ldns_buffer_position(qb);
                    if (l) {
                        void *p = ldns_buffer_export(qb);
                        if (p) {
                            memset(qb, 0, sizeof(ldns_buffer));
                            mem = mem_new(p, l, &sa, sl);
                        }
                    }
                }
                ldns_buffer_free(qb);
            }
            ldns_pkt_free(dp);
        }
    }
    ldns_rdf_deep_free(n);

    return mem;
}

int dns_lookup(const char *hostname, void (*callback)(void *, const char *), void *opaque) {
    net_event_t *net = map_front(__dns);
    if (net) {
        dns_entity_t *query = calloc(1, sizeof(dns_entity_t));
        if (query) {
            clock_gettime(CLOCK_REALTIME, &query->birth);
            query->host = strdup(hostname);
            query->callback = callback;
            query->opaque = opaque;
            query->id = ++__id & 0xffff ? __id : ++__id;
            mem_t *mem = dns_query(net->foo, query->id, hostname, LDNS_RR_TYPE_A, LDNS_RR_CLASS_IN, LDNS_RD);
            if (mem) {
                map_push_back(net->outgoing, mem);
                map_push(net->entities, query);
                net_event_update(net, NULL);
                return 0;
            }
            dns_destroy2(query);
        }
    }
    return -1;
}

void dns_callback(void *foo, const char *name) {
    log_alert("%s %s", foo, name);
}

static char hostname[256] = "127.0.0.1";

const app_t __g_app_DNS = {
    "DNS",
    "0.0.0.0",
    hostname,
    "0",
    dns_acquire,
    dns_release,
    dns_execute,
    NULL,
    NULL,
    {{dns_destroy, dns_compare}},
    {1, 0}
};
