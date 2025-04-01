/*
 * tcp.c
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
#include <netinet/tcp.h>

#include "net.h"
#include "log.h"

/**
 *
 */
static int tcp_open(const net_t *net, const char *host, const unsigned short port) {
    log_debug("%s: %s:%hu", __PRETTY_FUNCTION__, host, port);

    const int s = socket(net->family, net->type, net->protocol);
    if (s != -1) {
        const int yes = 1;
        const int not = 0;

        int flag = fcntl(s, F_GETFL);
        if (flag != -1) {
            flag |= O_NONBLOCK;
            if (fcntl(s, F_SETFL, flag) == -1) {
                close(s);
                return -1;
            }
        }

        setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
#ifdef SO_REUSEPORT
        setsockopt(s, SOL_SOCKET, SO_REUSEPORT, &yes, sizeof(int));
#endif
        setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, &not, sizeof(int));

        struct sockaddr_in la = {0};

        la.sin_addr.s_addr = inet_addr(host);
        la.sin_family = abs(net->family);
        la.sin_port = htons(port);

        if (bind(s, (struct sockaddr *) &la, sizeof(la)) != -1) {
            if (listen(s, 32) != -1) {
                return s;
            }
        }
        close(s);
    }
    return -1;
}

/**
 *
 */
static int tcp_connect(const net_t *net, const char *host, const unsigned short port) {
    log_debug("%s: %s:%hu", __PRETTY_FUNCTION__, host, port);

    const int s = socket(net->family, net->type, net->protocol);
    if (s != -1) {
        const int yes = 1;
        const int not = 0;

        int flag = fcntl(s, F_GETFL);
        if (flag != -1) {
            flag |= O_NONBLOCK;
            if (fcntl(s, F_SETFL, flag) == -1) {
                close(s);
                return -1;
            }
        }

        setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
#ifdef SO_REUSEPORT
        setsockopt(s, SOL_SOCKET, SO_REUSEPORT, &yes, sizeof(int));
#endif
        setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, &not, sizeof(int));

        struct sockaddr_in la = {0};
        struct sockaddr_in sa = {0};

        la.sin_addr.s_addr = INADDR_ANY;
        la.sin_family = net->family;
        la.sin_port = 0;

        sa.sin_addr.s_addr = inet_addr(host);
        sa.sin_family = abs(net->family);
        sa.sin_port = htons(port);

        if (bind(s, (struct sockaddr *) &la, sizeof(la)) != -1) {
            if (connect(s, (struct sockaddr *) &sa, sizeof(sa)) != -1)
                return s;

            switch (errno) {
                case EINPROGRESS:
                    return s;
                default:
                    break;
            }
        }
        close(s);
    }
    return -1;
}

/**
 *
 */
static int tcp_read(const int s, mem_t *m, void *) {
    if (m->end <= m->length) {
        const int n = recv(s, m->buffer + m->end, m->length - m->end, MSG_NOSIGNAL);
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
static int tcp_send(const int s, mem_t *m, void *) {
    if (m->done <= m->end) {
        const int n = send(s, m->buffer + m->done, m->end - m->done, MSG_NOSIGNAL);
        if (n != -1) {
            m->done += n;
            return n;
        }
    }
    return -1;
}

/**
 *
 */
static int tcp_accept(const int s, struct sockaddr *a, socklen_t *l) {
    const int c = accept(s, a, l);

    const struct sockaddr_in *sin = (struct sockaddr_in *) a;

    log_debug("%s: %i/%s:%hu", __PRETTY_FUNCTION__, c, inet_ntoa(sin->sin_addr), ntohs(sin->sin_port));

    return c;
}

/**
 *
 * @param s
 * @param yes
 * @return
 */
static int tcp_block(const int s, const int yes) {
    int flag = fcntl(s, F_GETFL);
    if (flag < 0)
        return -1;
    if (yes)
        flag |= O_NONBLOCK;
    else
        flag &= ~O_NONBLOCK;
    if (fcntl(s, F_SETFL, flag) < 0)
        return -1;
    return 0;
}

/**
 *
 */
const net_t __g_net_TCP = {
    "TCP",
    0,
    IPPROTO_TCP,
    AF_INET,
    SOCK_STREAM,
    NULL,
    NULL,
    tcp_open,
    tcp_read,
    tcp_send,
    tcp_block,
    tcp_connect,
    tcp_accept,
    NULL,
    NULL,
    NULL,
};
