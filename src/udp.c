/*
 * udp.c
 *
 *  Created on: Sep 24, 2013
 *      Author: shved
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <netinet/udp.h>

#include <assert.h>

#include "net.h"
#include "log.h"

/**
 *
 */
static int udp_open(const net_t *net, const char *host, const unsigned short port) {
    log_debug("%s: %s:%hu", __PRETTY_FUNCTION__, host, port);

    int s = socket(net->family, net->type, net->protocol);
    if (s != -1) {
        const int yes = 1;

        setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

        struct sockaddr_in la = {0};

        la.sin_addr.s_addr = inet_addr(host);
        la.sin_family = abs(net->family);
        la.sin_port = htons(port);

        if (bind(s, (struct sockaddr *) &la, sizeof(la)) != -1) {
            return s;
        }
        close(s);
    }
    return -1;
}

/**
 *
 */
static int udp_read(const int s, mem_t *m, void *) {
    if (m->end) {
        log_dump(m->buffer + m->done, m->end - m->done);
        m->begin = 0;
        m->done = 0;
        m->end = 0;
    }
    if (m->end < m->length) {
        const int n = recvfrom(s, m->buffer + m->end, m->length - m->end, MSG_NOSIGNAL, &m->peer->a, &m->peer->l);
        if (n != -1) {
            m->end += n;
            return n;
        }
    }
    return -1;
}

/**
 *
 */
static int udp_send(const int s, mem_t *m, void *) {
    if (m->done < m->end) {
        const int n = sendto(s, m->buffer + m->done, m->end - m->done, MSG_NOSIGNAL, &m->peer->a, m->peer->l);
        if (n != -1) {
            m->done += n;
            return n;
        }
    } else if (m->done == m->end)
        return 0;
    return -1;
}

/**
 *
 */
const net_t __g_net_UDP = {
    "UDP",
    0,
    IPPROTO_UDP,
    AF_INET,
    SOCK_DGRAM,
    NULL,
    NULL,
    udp_open,
    udp_read,
    udp_send,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
};
