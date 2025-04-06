/*
 * sip.c
 *
 *  Created on: Sep 25, 2013
 *      Author: shved
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ctype.h>

#include "aor.h"
#include "app.h"
#include "log.h"

static struct osip *__sip = NULL;

enum {
    SIP_APPLICATION_EVENT,
    SIP_APPLICATION_CLOSURE,
    SIP_APPLICATION_OPAQUE,
    SIP_APPLICATION_TIMEOUT,
    SIP_APPLICATION_RETRY_COUNT,
};

#define CRLFCRLF			"\r\n\r\n"
#define LFLF				"\n\n"

#define IS_XIST(a)			((a)->ctx_type == IST || (a)->ctx_type == NIST)
#define IS_XICT(a)			((a)->ctx_type == ICT || (a)->ctx_type == NICT)

#define osip_message_get_application_data(__m)		__message_get_application_data(__m)
#define osip_message_set_application_data(__m, __q)	__message_set_application_data(__m, __q)

#define osip_transaction_get_event(__a) 			__transaction_get_application_data(__a, SIP_APPLICATION_EVENT)
#define osip_transaction_set_event(__a, __n)		__transaction_set_application_data(__a, SIP_APPLICATION_EVENT, __n)
#define osip_transaction_get_closure(__a) 			__transaction_get_application_data(__a, SIP_APPLICATION_CLOSURE)
#define osip_transaction_set_closure(__a, __n)		__transaction_set_application_data(__a, SIP_APPLICATION_CLOSURE, __n)
#define osip_transaction_get_opaque(__a) 			__transaction_get_application_data(__a, SIP_APPLICATION_OPAQUE)
#define osip_transaction_set_opaque(__a, __n)		__transaction_set_application_data(__a, SIP_APPLICATION_OPAQUE, __n)
#define osip_transaction_get_timeout(__a) 			__transaction_get_application_data(__a, SIP_APPLICATION_TIMEOUT)
#define osip_transaction_set_timeout(__a, __n)		__transaction_set_application_data(__a, SIP_APPLICATION_TIMEOUT, __n)
#define osip_transaction_get_retry(__a) 			__transaction_get_application_data(__a, SIP_APPLICATION_RETRY_COUNT)
#define osip_transaction_set_retry(__a, __n)		__transaction_set_application_data(__a, SIP_APPLICATION_RETRY_COUNT, __n)

extern osip_message_t *ict_create_ack(osip_transaction_t *, osip_message_t *);

extern osip_event_t *__osip_event_new(type_t, int);

/**
 *
 */
static osip_event_t *__osip_event_new_id(osip_message_t *m, const int id) {
    osip_event_t *event = osip_new_outgoing_sipmessage(m);
    event->transactionid = id;
    return event;
}

extern char *sip_reason_by_code(int);

/**
 *
 */
static void *__message_get_application_data(osip_message_t *m) {
    return m ? m->application_data : NULL;
}

/**
 *
 */
static void __message_set_application_data(osip_message_t *m, void *a) {
    if (m)
        m->application_data = a;
}

/**
 *
 */
static void *__transaction_get_application_data(osip_transaction_t *a, const unsigned n) {
    if (!a)
        return NULL;
    switch (n) {
        case SIP_APPLICATION_EVENT:
            return osip_transaction_get_reserved1(a);
        case SIP_APPLICATION_CLOSURE:
            return osip_transaction_get_reserved2(a);
        case SIP_APPLICATION_OPAQUE:
            return osip_transaction_get_reserved3(a);
        case SIP_APPLICATION_TIMEOUT:
            return osip_transaction_get_reserved4(a);
        case SIP_APPLICATION_RETRY_COUNT:
            return osip_transaction_get_reserved5(a);
        default:
            return NULL;
    }
}

/**
 *
 */
static void __transaction_set_application_data(osip_transaction_t *a, const unsigned n, void *q) {
    if (!a)
        return;
    switch (n) {
        case SIP_APPLICATION_EVENT:
            osip_transaction_set_reserved1(a, q);
            break;
        case SIP_APPLICATION_CLOSURE:
            osip_transaction_set_reserved2(a, q);
            break;
        case SIP_APPLICATION_OPAQUE:
            osip_transaction_set_reserved3(a, q);
            break;
        case SIP_APPLICATION_TIMEOUT:
            osip_transaction_set_reserved4(a, q);
            break;
        case SIP_APPLICATION_RETRY_COUNT:
            osip_transaction_set_reserved5(a, q);
            break;
        default:
            return;
    }
}

/**
 *
 */
static const char *sip_callback_type(const osip_message_callback_type_t type) {
    static struct {
        osip_message_callback_type_t type;
        char *name;
    } *m, types[] = {

                __type_name(OSIP_ICT_INVITE_SENT),
                __type_name(OSIP_ICT_INVITE_SENT_AGAIN),
                __type_name(OSIP_ICT_ACK_SENT),
                __type_name(OSIP_ICT_ACK_SENT_AGAIN),
                __type_name(OSIP_ICT_STATUS_1XX_RECEIVED),
                __type_name(OSIP_ICT_STATUS_2XX_RECEIVED),
                __type_name(OSIP_ICT_STATUS_2XX_RECEIVED_AGAIN),
                __type_name(OSIP_ICT_STATUS_3XX_RECEIVED),
                __type_name(OSIP_ICT_STATUS_4XX_RECEIVED),
                __type_name(OSIP_ICT_STATUS_5XX_RECEIVED),
                __type_name(OSIP_ICT_STATUS_6XX_RECEIVED),
                __type_name(OSIP_ICT_STATUS_3456XX_RECEIVED_AGAIN),

                __type_name(OSIP_IST_INVITE_RECEIVED),
                __type_name(OSIP_IST_INVITE_RECEIVED_AGAIN),
                __type_name(OSIP_IST_ACK_RECEIVED),
                __type_name(OSIP_IST_ACK_RECEIVED_AGAIN),
                __type_name(OSIP_IST_STATUS_1XX_SENT),
                __type_name(OSIP_IST_STATUS_2XX_SENT),
                __type_name(OSIP_IST_STATUS_2XX_SENT_AGAIN),
                __type_name(OSIP_IST_STATUS_3XX_SENT),
                __type_name(OSIP_IST_STATUS_4XX_SENT),
                __type_name(OSIP_IST_STATUS_5XX_SENT),
                __type_name(OSIP_IST_STATUS_6XX_SENT),
                __type_name(OSIP_IST_STATUS_3456XX_SENT_AGAIN),

                __type_name(OSIP_NICT_REGISTER_SENT),
                __type_name(OSIP_NICT_BYE_SENT),
                __type_name(OSIP_NICT_OPTIONS_SENT),
                __type_name(OSIP_NICT_INFO_SENT),
                __type_name(OSIP_NICT_CANCEL_SENT),
                __type_name(OSIP_NICT_NOTIFY_SENT),
                __type_name(OSIP_NICT_SUBSCRIBE_SENT),
                __type_name(OSIP_NICT_UNKNOWN_REQUEST_SENT),
                __type_name(OSIP_NICT_REQUEST_SENT_AGAIN),
                __type_name(OSIP_NICT_STATUS_1XX_RECEIVED),
                __type_name(OSIP_NICT_STATUS_2XX_RECEIVED),
                __type_name(OSIP_NICT_STATUS_2XX_RECEIVED_AGAIN),
                __type_name(OSIP_NICT_STATUS_3XX_RECEIVED),
                __type_name(OSIP_NICT_STATUS_4XX_RECEIVED),
                __type_name(OSIP_NICT_STATUS_5XX_RECEIVED),
                __type_name(OSIP_NICT_STATUS_6XX_RECEIVED),
                __type_name(OSIP_NICT_STATUS_3456XX_RECEIVED_AGAIN),

                __type_name(OSIP_NIST_REGISTER_RECEIVED),
                __type_name(OSIP_NIST_BYE_RECEIVED),
                __type_name(OSIP_NIST_OPTIONS_RECEIVED),
                __type_name(OSIP_NIST_INFO_RECEIVED),
                __type_name(OSIP_NIST_CANCEL_RECEIVED),
                __type_name(OSIP_NIST_NOTIFY_RECEIVED),
                __type_name(OSIP_NIST_SUBSCRIBE_RECEIVED),

                __type_name(OSIP_NIST_UNKNOWN_REQUEST_RECEIVED),
                __type_name(OSIP_NIST_REQUEST_RECEIVED_AGAIN),
                __type_name(OSIP_NIST_STATUS_1XX_SENT),
                __type_name(OSIP_NIST_STATUS_2XX_SENT),
                __type_name(OSIP_NIST_STATUS_2XX_SENT_AGAIN),
                __type_name(OSIP_NIST_STATUS_3XX_SENT),
                __type_name(OSIP_NIST_STATUS_4XX_SENT),
                __type_name(OSIP_NIST_STATUS_5XX_SENT),
                __type_name(OSIP_NIST_STATUS_6XX_SENT),
                __type_name(OSIP_NIST_STATUS_3456XX_SENT_AGAIN),

                __type_name(OSIP_ICT_STATUS_TIMEOUT),
                __type_name(OSIP_NICT_STATUS_TIMEOUT),

                __type_name_END,
            };

    for (m = types; m->name; ++m)
        if (m->type == type)
            return m->name;

    return NULL;
}

/**
 *
 */
static const char *sip_transaction_type(const osip_transaction_t *a) {
    static struct {
        osip_fsm_type_t type;
        char *name;
    } *m, types[] = {
                __type_name(ICT), __type_name(IST), __type_name(NICT), __type_name(NIST),
                __type_name_END,
            };

    for (m = types; m->name; ++m)
        if (m->type == a->ctx_type)
            return m->name;

    return NULL;
}

/**
 *
 */
static const char *sip_transaction_state(const osip_transaction_t *a) {
    static struct {
        state_t state;
        char *name;
    } *m, states[] = {

                __type_name(ICT_PRE_CALLING),
                __type_name(ICT_CALLING),
                __type_name(ICT_PROCEEDING),
                __type_name(ICT_COMPLETED),
                __type_name(ICT_TERMINATED),

                __type_name(IST_PRE_PROCEEDING),
                __type_name(IST_PROCEEDING),
                __type_name(IST_COMPLETED),
                __type_name(IST_CONFIRMED),
                __type_name(IST_TERMINATED),

                __type_name(NICT_PRE_TRYING),
                __type_name(NICT_TRYING),
                __type_name(NICT_PROCEEDING),
                __type_name(NICT_COMPLETED),
                __type_name(NICT_TERMINATED),

                __type_name(NIST_PRE_TRYING),
                __type_name(NIST_TRYING),
                __type_name(NIST_PROCEEDING),
                __type_name(NIST_COMPLETED),
                __type_name(NIST_TERMINATED),

                __type_name_END,
            };

    for (m = states; m->name; ++m)
        if (m->state == a->state)
            return m->name;

    return NULL;
}

/**
 *
 */
static int sip_ip_address_and_port_set(struct sockaddr *a, const char *ip, int port) {
    struct addrinfo *i = NULL;

    int n = getaddrinfo(ip, NULL, NULL, &i);
    if (n != 0)
        return -1;

    a->sa_family = i->ai_family;

    freeaddrinfo(i);

    switch (a->sa_family) {
        case AF_INET:
            ((struct sockaddr_in *) a)->sin_port = htons(port);
            if (1 == inet_pton(a->sa_family, ip, &(((struct sockaddr_in *) a)->sin_addr)))
                return sizeof(struct sockaddr_in);
            break;
        case AF_INET6:
            ((struct sockaddr_in6 *) a)->sin6_port = htons(port);
            if (1 == inet_pton(a->sa_family, ip, &(((struct sockaddr_in6 *) a)->sin6_addr)))
                return sizeof(struct sockaddr_in6);
            break;
        default:
            break;
    }
    return -1;
}

/**
 *
 */
static int sip_ip_address_and_port_get(const struct sockaddr *a, char *ip, size_t len) {
    switch (a->sa_family) {
        case AF_INET:
            if (inet_ntop(a->sa_family, &(((struct sockaddr_in *) a)->sin_addr), ip, len))
                return ntohs(((struct sockaddr_in *) a)->sin_port);
            break;
        case AF_INET6:
            if (inet_ntop(a->sa_family, &(((struct sockaddr_in6 *) a)->sin6_addr), ip, len))
                return ntohs(((struct sockaddr_in6 *) a)->sin6_port);
            break;
        default:
            break;
    }
    return -1;
}

/**
 *
 */
void sip_dump(const char *p, const char *q, const unsigned l) {
    int n = 0;
    int b = 0;

    char *m = strndup(q, l);
    if (!m)
        return;

    while (n < l) {
        if (m[n] == '\r' || m[n] == '\n') {
            if (b < n) {
                const char c = m[n];

                m[n] = 0;
                log_info("%s %s", p, m + b);
                m[n] = c;
                p = " ";
            } else if (b && m[b - 1] == '\n') {
                log_info("%s", p);
                p = " ";
            }
            n++;
            b = n;

            continue;
        }
        n++;
    }
    if (b < n) {
        log_info("%s %s", p, m + b);
    }
    free(m);
}

#define sip_dump_in(m, l) sip_dump("<", m, l)
#define sip_dump_to(m, l) sip_dump(">", m, l)

/**
 *
 */
int sip_from_is_valid(osip_from_t *f) {
    if (!f)
        return 0;

    if (!f->url)
        return 0;

    if (!f->url->scheme)
        return 0;

    if (!f->url->username)
        return 0;

    if (!strcmp(f->url->username, "*"))
        return 0;

    if (!f->url->host)
        return 0;

    return 1;
}

/**
 *
 */
int sip_message_is_valid(osip_message_t *m) {
    if (!m)
        return 0;

    if (!osip_list_size(&m->vias))
        return 0;

    if (!sip_from_is_valid(m->from))
        return 0;

    if (!sip_from_is_valid(m->to))
        return 0;

    if (!m->call_id)
        return 0;

    if (!m->cseq)
        return 0;

    return 1;
}

/**
 *
 */
static osip_event_t *sip_read(mem_t *chunk, map_t *output) {
    if (chunk->done == chunk->begin) {
        char *end = memmem(chunk->buffer + chunk->done, chunk->end - chunk->done, CRLFCRLF, 4);
        if (!end) {
            end = memmem(chunk->buffer + chunk->done, chunk->end - chunk->done, LFLF, 2);
            if (end)
                chunk->done = end - chunk->buffer + 2;
            else
                return NULL; /* not complete yet */
        } else
            chunk->done = end - chunk->buffer + 4;

        const char *head = chunk->buffer + chunk->begin;
        int length = 0;
        for (;;) {
            const char *len = strncasestr(head, "\n" CONTENT_LENGTH, end - head);
            if (!len) {
                len = strncasestr(head, "\n" CONTENT_LENGTH_SHORT, end - head);
                if (!len)
                    break; /* no content length */
                len++;
                len++;
            } else
                len += 15;

            while (*len == ' ' || *len == '\t')
                len++;

            if (*len != ':') {
                head = len;
                continue;
            } else
                len++;

            while (*len == ' ' || *len == '\t')
                len++;

            length = atoi(len);

            break; /* found */
        }
        chunk->done += length;
    }

    if (chunk->done > chunk->end)
        return NULL;

    char ip_addr[INET6_ADDRSTRLEN];

    const int port = sip_ip_address_and_port_get(&chunk->peer->a, ip_addr, sizeof(ip_addr));
    if (port == -1) {
        chunk->begin = chunk->done;
        return NULL;
    }

    log_debug(FL_YELLOW "%s: %s:%i" FD_NORMAL, __PRETTY_FUNCTION__, ip_addr, port);

    osip_event_t *e = osip_parse(chunk->buffer + chunk->begin, chunk->done - chunk->begin);
    if (e && sip_message_is_valid(e->sip)) {
        const int n = osip_message_fix_last_via_header(e->sip, ip_addr, port);
        if (n)
            log_dump(chunk->buffer + chunk->begin, chunk->done - chunk->begin);
        else
            sip_dump_in(chunk->buffer + chunk->begin, chunk->done - chunk->begin);
    } else {
        if (!e && chunk->done - chunk->begin == 4 && !memcmp(chunk->buffer + chunk->begin, CRLFCRLF, 4)) {
            mem_t *mem = mem_new(memcpy(malloc(2), CRLF, 2), 2, &chunk->peer->a, chunk->peer->l);
            if (mem)
                map_push_back(output, mem);
        } else
            log_dump(chunk->buffer + chunk->begin, chunk->done - chunk->begin);
        if (e)
            osip_event_free(e);
        e = NULL;
    }
    chunk->begin = chunk->done;

    return e;
}

/**
 *
 */
static int sip_send(osip_transaction_t *a, osip_message_t *m, char *host, const int port, int foo) {
    log_debug(FL_GREEN "%s: %s:%i" FD_NORMAL, __PRETTY_FUNCTION__, host, port);

    net_event_t *sip = osip_transaction_get_event(a);
    if (!sip)
        return -1;

    osip_list_special_free(&m->allows, (void (*)(void *)) &osip_allow_free); {
        char date[64 + 1];
        const time_t now = time(NULL);
        strftime(date, sizeof(date), "%a, %d %b %Y %T %Z", gmtime(&now));
        osip_message_replace_header(m, "Date", date);
    }

    char *message = NULL;
    size_t length = 0;

    if (0 != osip_message_to_str(m, &message, &length))
        return -1;
    else
        sip_dump_to(message, length);

    struct sockaddr sa = {0};

    const int l = sip_ip_address_and_port_set(&sa, host, port);
    if (l != -1) {
        mem_t *mem = mem_new(message, length, &sa, l);
        if (mem) {
            map_push_back(sip->outgoing, mem);
            return 0;
        }
        mem_free(mem);
    }
    osip_free(message);

    return -1;
}

/**
 *
 */
int sip_call(osip_transaction_t *a, osip_event_t *e) {
    if (!e)
        return -1;

    switch (osip_transaction_execute(a, e)) {
        case 1: /* continue */
            log_debug(FL_RED "%s: %i:%s" FD_NORMAL,
                      __PRETTY_FUNCTION__, a->transactionid, sip_transaction_state(a));

            switch (a->state) {
                case IST_PRE_PROCEEDING:
                case IST_PROCEEDING:
                case IST_COMPLETED:
                    break;
                case IST_CONFIRMED:
                case IST_TERMINATED:
                    sip_delete(a);
                    return -1;

                case ICT_PRE_CALLING:
                case ICT_CALLING:
                case ICT_PROCEEDING:
                    break;
                case ICT_TERMINATED:
                case ICT_COMPLETED:
                    sip_delete(a);
                    return -1;

                case NIST_PRE_TRYING:
                case NIST_TRYING:
                    break;
                case NIST_COMPLETED:
                case NIST_TERMINATED:
                    sip_delete(a);
                    return -1;

                case NICT_PRE_TRYING:
                case NICT_TRYING:
                    break;
                case NICT_COMPLETED:
                case NICT_TERMINATED:
                    sip_delete(a);
                    return -1;

                default:
                    log_alert(FL_WHITE "%s: [%s] NOT HANDLED IN SWITCH %s:%u" FD_NORMAL,
                              __PRETTY_FUNCTION__, sip_transaction_state(a), //
                              __FILE__, __LINE__);
                    break;
            }
            break;
        case 0: /* kill 'em all */
            break;
        default: /* failed */
            log_error("%s: FAILED", __PRETTY_FUNCTION__);
            break;
    }
    return 0;
}

/**
 *
 */
osip_transaction_t *sip_loop(osip_event_t *e, void *q, ...) {
    if (!__sip)
        return NULL;

    osip_transaction_t *a = __osip_find_transaction(__sip, e, 1);
    if (!a) {
        a = osip_create_transaction(__sip, e);
        if (a) {
            osip_transaction_add_event(a, e);
            va_list ap;
            va_start(ap, q);
            int i = 0;
            while (q) {
                __transaction_set_application_data(a, i, q);
                q = va_arg(ap, void *);
                ++i;
            }
            va_end(ap);
        } else
            osip_event_free(e);
    }

    const net_event_t *net = osip_transaction_get_event(a);
    if (net)
        map_push(net->entities, a);

    while (a) {
        osip_event_t *o = osip_fifo_tryget(a->transactionff);
        if (!o)
            break;
        if (sip_call(a, o))
            break;
    }

    return a;
}

/**
 * http://tools.ietf.org/html/rfc5658
 */
static void sip_message_add_record_route(osip_message_t *m, net_event_t *net) {
    if (MSG_IS_REQUEST(m)) {
        osip_uri_t *url = NULL;
        osip_uri_init(&url);
        osip_uri_set_scheme(url, osip_strdup(m->req_uri->scheme));
        osip_uri_set_host(url, osip_strdup(net->app->hostname));
        osip_uri_set_port(url, osip_strdup(net->app->port));
        osip_uri_param_add(&url->url_params, osip_strdup("transport"), osip_strdup(net->net->name));
        osip_uri_param_add(&url->url_params, osip_strdup("lr"), NULL);

        osip_record_route_t *route = NULL;
        osip_record_route_init(&route);
        osip_record_route_set_url(route, url);
        osip_list_add(&m->record_routes, route, 0);
    }
}

/**
 *
 */
static void sip_message_fix_transport(osip_message_t *m, net_event_t *net) {
    osip_list_iterator_t next;
    osip_contact_t *contact = osip_list_get_first(&m->contacts, &next);
    while (osip_list_iterator_has_elem(next)) {
        if (contact && contact->url) {
            osip_uri_param_t *transport = NULL;
            osip_uri_param_get_byname(&contact->url->url_params, "transport", &transport);
            if (!transport) {
                osip_uri_set_transport(contact->url, osip_strdup(net->net->name));
            }
        }
        contact = osip_list_get_next(&next);
    }
}

/**
 *
 */
static int sip_execute(net_event_t *net) {
    log_debug("%s: %i/%s/%s", __PRETTY_FUNCTION__, net->event->ev_fd, net->net->name, net->app->name);

    osip_event_t *e = sip_read(net->incoming, net->outgoing);
    if (!e)
        return -1;

    sip_message_add_record_route(e->sip, net);
    sip_message_fix_transport(e->sip, net);

    if (!sip_loop(e, net, NULL))
        return -1;
    return 0;
}

/**
 *
 */
static void sip_message_remove_top_via(osip_message_t *q) {
    if (q) {
        osip_via_t *via = osip_list_get(&q->vias, 0);
        if (via) {
            osip_via_free(via);
            osip_list_remove(&q->vias, 0);
        }
    }
}

/**
 *
 */
static void sip_response(osip_message_t *m, const int code) {
    if (!m)
        return;

    osip_free(m->sip_method);
    osip_free(m->sip_version);
    osip_free(m->reason_phrase);
    if (m->req_uri != NULL)
        osip_uri_free(m->req_uri);

    osip_message_set_status_code(m, code);
    osip_message_set_reason_phrase(m, sip_reason_by_code(code));
    osip_message_set_version(m, osip_strdup(SIP_DEFAULT_VERSION));

    osip_list_special_free(&m->authorizations, (void (*)(void *)) &osip_authorization_free);
    osip_list_special_free(&m->accepts, (void (*)(void *)) &osip_accept_free);
    osip_list_special_free(&m->accept_encodings, (void (*)(void *)) &osip_accept_encoding_free);
    osip_list_special_free(&m->accept_languages, (void (*)(void *)) &osip_accept_language_free);
    osip_list_special_free(&m->alert_infos, (void (*)(void *)) &osip_alert_info_free);
    osip_list_special_free(&m->allows, (void (*)(void *)) &osip_allow_free);
    osip_list_special_free(&m->authentication_infos, (void (*)(void *)) &osip_authentication_info_free);
    osip_list_special_free(&m->call_infos, (void (*)(void *)) &osip_call_info_free);
    osip_list_special_free(&m->content_encodings, (void (*)(void *)) &osip_content_encoding_free);
    osip_list_special_free(&m->error_infos, (void (*)(void *)) &osip_error_info_free);
    osip_list_special_free(&m->proxy_authentication_infos,
                           (void (*)(void *)) &osip_proxy_authentication_info_free);

    if (m->content_length != NULL)
        osip_content_length_free(m->content_length);
    if (m->content_type != NULL)
        osip_content_type_free(m->content_type);
    if (m->mime_version != NULL)
        osip_mime_version_free(m->mime_version);
    osip_list_special_free(&m->proxy_authenticates, (void (*)(void *)) &osip_proxy_authenticate_free);
    osip_list_special_free(&m->proxy_authorizations, (void (*)(void *)) &osip_proxy_authorization_free);
    osip_list_special_free(&m->www_authenticates, (void (*)(void *)) &osip_www_authenticate_free);
    osip_list_special_free(&m->headers, (void (*)(void *)) &osip_header_free);
    osip_list_special_free(&m->bodies, (void (*)(void *)) &osip_body_free);
    osip_free(m->message);

    m->req_uri = NULL;
    m->sip_method = NULL;
    m->mime_version = NULL;
    m->content_type = NULL;
    m->content_length = NULL;
    m->message = NULL;
    m->message_property = 2;
}

/**
 *
 */
static osip_message_t *sip_response_build(osip_message_t *q) {
    osip_message_t *m = NULL;
    osip_message_init(&m);
    osip_message_set_version(m, osip_strdup(SIP_DEFAULT_VERSION));

    osip_from_clone(q->from, &m->from);
    osip_to_clone(q->to, &m->to);
    osip_call_id_clone(q->call_id, &m->call_id);
    osip_cseq_clone(q->cseq, &m->cseq);
    osip_list_clone(&q->vias, &m->vias, (int (*)(void *, void **)) &osip_via_clone);
    osip_list_clone(&q->record_routes, &m->record_routes, (int (*)(void *, void **)) &osip_record_route_clone);

    return m;
}

/**
 *
 */
static osip_message_t *sip_response_new(osip_transaction_t *a, osip_message_t *q, const int code) {
    osip_message_t *m = sip_response_build(q);
    if (m) {
        osip_message_set_status_code(m, code);
        osip_message_set_reason_phrase(m, sip_reason_by_code(code));
    }
    return m;
}

/**
 *
 */
static void sip_finalize_invite(osip_transaction_t *a, osip_message_t *m, const net_event_t *net) {
    osip_list_special_free(&m->authorizations, (void (*)(void *)) &osip_authorization_free);
    osip_list_special_free(&m->headers, (void (*)(void *)) &osip_header_free);

    if (m->status_code != SIP_OK)
        return;

    osip_via_t *via = NULL;
    osip_message_get_via(m, 0, &via);
    if (via && strcasecmp(via->protocol, net->net->name) != 0)
        via = NULL;

    aor_update(a->orig_request, -1, .0f, via ? net->id : 0);
}

/**
 *
 */
static void sip_finalize_subscribe(osip_transaction_t *a, osip_message_t *m, const net_event_t *net) {
    osip_list_special_free(&m->authorizations, (void (*)(void *)) &osip_authorization_free);
    osip_list_special_free(&m->headers, (void (*)(void *)) &osip_header_free);

    if (m->status_code != SIP_OK)
        return;

    char contact[512];
    snprintf(contact, sizeof(contact), "%s:%s:%s", net->app->name, net->app->hostname, net->app->port);
    osip_tolower(contact);
    osip_message_set_contact(m, contact);
}

/**
 *
 */
static void sip_finalize_registrate(osip_transaction_t *a, osip_message_t *m, const net_event_t *net) {
    osip_list_special_free(&m->authorizations, (void (*)(void *)) &osip_authorization_free);
    osip_list_special_free(&m->contacts, (void (*)(void *)) &osip_contact_free);
    osip_list_special_free(&m->headers, (void (*)(void *)) &osip_header_free);

    if (m->status_code != SIP_OK)
        return;

    osip_via_t *via = NULL;
    osip_message_get_via(m, 0, &via);
    if (via && strcasecmp(via->protocol, net->net->name) != 0)
        via = NULL;

    aor_t *from = aor_update(a->orig_request, SIP_DEFAULT_EXPIRES, .5f, via ? net->id : 0);
    if (from) {
        map_iter_t *iter = map_iter_end(&from->contact);
        while (*iter) {
            const aor_record_t *next = map_iter_up(&from->contact, &iter);
            if (next) {
                osip_contact_t *contact = NULL;
                osip_contact_clone(next->contact, &contact);
                osip_list_add(&m->contacts, contact, 0);
            }
        }
        return;
    }
    sip_response(m, SIP_FORBIDDEN);
}

/**
 *
 */
void sip_finalize_failure(const uuid_t id) {
    if (!__sip)
        return;

    osip_event_t *e = __osip_event_new(SND_STATUS_3456XX, cmd_session_find(id));
    if (e) {
        osip_list_t *transactions[] = {
            &__sip->osip_ist_transactions,
            &__sip->osip_nist_transactions,
            &__sip->osip_ict_transactions,
            &__sip->osip_nict_transactions,
            NULL
        }, **next = transactions;
        while (*next) {
            osip_transaction_t *a = osip_transaction_find(*next, e);
            if (a) {
                net_event_t *net = osip_transaction_get_event(a);
                if (net) {
                    e->sip = sip_response_new(a, a->orig_request, SIP_INTERNAL_SERVER_ERROR);
                    if (!e->sip) {
                        sip_delete(a);
                        break;
                    }
                    sip_call(a, e);
                    net_event_update(net, NULL);
                    return;
                }
            }
            next++;
        }
        return osip_event_free(e);
    }
}

/**
 *
 * @param m
 * @param id
 */
void sip_finalize(osip_message_t *m, const uuid_t id) {
    if (!__sip)
        return;

    osip_event_t *e = __osip_event_new_id(m, cmd_session_find(id));
    if (e) {
        osip_transaction_t *a = __osip_find_transaction(__sip, e, 0);
        if (a) {
            net_event_t *n = osip_transaction_get_event(a);
            if (n) {
                if (MSG_IS_RESPONSE_FOR(m, SIP_METHOD_REGISTER))
                    sip_finalize_registrate(a, m, n);
                else if (MSG_IS_RESPONSE_FOR(m, SIP_METHOD_INVITE))
                    sip_finalize_invite(a, m, n);
                else if (MSG_IS_RESPONSE_FOR(m, SIP_METHOD_SUBSCRIBE))
                    sip_finalize_subscribe(a, m, n);

                sip_call(a, e);
                return net_event_update(n, NULL);
            }
        }
        return osip_event_free(e);
    }
    return osip_message_free(m);
}

/**
 *
 */
static int sip_relay(osip_message_t *q, aor_record_t *to, const Sip__Message_Closure closure, void *opaque,
                     const uuid_t id) {
    if (!q || !to || !closure || uuid_is_null(id))
        return -1;

    char max_forward[32] = SIP_DEFAULT_MAX_FORWARD;
    osip_header_t *forward = NULL;
    osip_message_get_max_forwards(q, 0, &forward);
    if (forward) {
        int n = osip_atoi(forward->hvalue);
        if (n > 1) {
            sprintf(max_forward, "%i", --n);
        } else
            return -1;
    }
    osip_message_replace_header(q, "Max-Forwards", max_forward);

    log_debug("%s: %s:%s/%s/%s",
              __PRETTY_FUNCTION__, to->route->host, to->route->port, to->protocol, to->route->scheme);

    net_event_t *net = net_event_find_by_id(to->id);
    if (net == NULL) {
        net = net_event_connect(to->protocol, to->route->scheme, to->route->host, net_port(to->route));
    }
    if (net) {
        osip_via_t *via = osip_list_get(&q->vias, 0);
        if (via) {
            osip_uri_param_t *branch = NULL;
            osip_via_param_get_byname(via, "branch", &branch);
            osip_via_init(&via);
            osip_via_set_version(via, osip_strdup("2.0"));
            osip_via_set_protocol(via, osip_strdup(net->net->name));
            osip_via_set_host(via, osip_strdup(net->app->hostname));
            osip_via_set_port(via, osip_strdup(net->app->port));
            osip_via_set_branch(via, osip_strdup(branch->gvalue));
            osip_uri_param_add(&via->via_params, osip_strdup("rport"), NULL);
            osip_list_add(&q->vias, via, 0);

            osip_uri_t *url = NULL;
            osip_uri_init(&url);
            osip_uri_set_scheme(url, osip_strdup(to->route->scheme));
            osip_uri_set_host(url, osip_strdup(to->route->host));
            osip_uri_set_port(url, osip_strdup(to->route->port));
            osip_uri_param_add(&url->url_params, osip_strdup("transport"), osip_strdup(net->net->name));
            osip_uri_param_add(&url->url_params, osip_strdup("lr"), NULL);

            osip_route_t *route = NULL;
            osip_route_init(&route);
            osip_route_set_url(route, url);
            osip_list_add(&q->routes, route, 0);

            sip_message_add_record_route(q, net);

            osip_event_t *e = osip_new_outgoing_sipmessage(q);
            if (e) {
                const osip_transaction_t *a = sip_loop(e, net, closure, opaque, NULL);
                if (a) {
                    const struct timeval timeout = {.tv_sec = 64 * DEFAULT_T1 / 1000, .tv_usec = 0};
                    if (net_is_connected(net))
                        net_event_update(net, &timeout);
                    else
                        net_event_update(net, NULL);
                    return cmd_session_add(a->transactionid, id);
                }
            }
            osip_list_remove(&q->vias, 0);
            osip_via_free(via);
        }
        if (net_is_connected(net))
            net_event_free(net);
    }
    return -1;
}

/**
 *
 */
void sip_proxy(osip_message_t *q, const Sip__Message_Closure closure, void *opaque, const uuid_t id) {
    const time_t now = time(0);

    log_debug(FL_CYAN "%s: %s" FD_NORMAL, __PRETTY_FUNCTION__, q->sip_method);

    osip_list_special_free(&q->authorizations, (void (*)(void *)) &osip_authorization_free);

    aor_t *to = aor_find(q->to);
    if (to) {
        map_iter_t *iter = map_iter_begin(&to->contact);
        while (*iter) {
            aor_record_t *next = map_iter_down(&to->contact, &iter);
            if (next && next->route) {
                if (MSG_IS_INVITE(q) && next->expired < now)
                    continue;
                if (sip_relay(q, next, closure, opaque, id))
                    break;
                return;
            }
        }
        if (*iter)
            sip_response(q, SIP_INTERNAL_SERVER_ERROR);
        else
            sip_response(q, SIP_GONE);
    } else
        sip_response(q, SIP_FORBIDDEN);

    cmd_finalize(q, closure, opaque, id);

    osip_message_free(q);
}

/**
 *
 */
static osip_message_t *sip_response_join(osip_message_t *m1, osip_message_t *m2) {
    if (m1) {
        osip_message_set_application_data(m1, m2);
        return m1;
    }
    return m2;
}

/**
 *
 */
static osip_message_t *sip_callback_register(osip_transaction_t *a, osip_message_t *q) {
    const int n = cmd_initiate_registration(a, q);
    switch (n) {
        case 0:
            return NULL;
        case -1:
            return sip_response_new(a, q, SIP_SERVICE_UNAVAILABLE);
        default:
            return sip_response_new(a, q, SIP_INTERNAL_SERVER_ERROR);
    }
}

/**
 *
 */
static osip_message_t *sip_callback_invite(osip_transaction_t *a, osip_message_t *q) {
    const int n = cmd_initiate_invite(a, q);
    switch (n) {
        case 0:
            return sip_response_join(sip_response_new(a, q, SIP_TRYING), sip_response_new(a, q, SIP_RINGING));
        case -1:
            return sip_response_new(a, q, SIP_SERVICE_UNAVAILABLE);
        default:
            return sip_response_new(a, q, SIP_INTERNAL_SERVER_ERROR);
    }
}

/**
 *
 */
static osip_message_t *sip_callback_options(osip_transaction_t *a, osip_message_t *q) {
    const int n = cmd_initiate_options(a, q);
    switch (n) {
        case 0:
            return NULL;
        case -1:
            return sip_response_new(a, q, SIP_SERVICE_UNAVAILABLE);
        default:
            return sip_response_new(a, q, SIP_INTERNAL_SERVER_ERROR);
    }
}

/**
 *
 */
static osip_message_t *sip_callback_cancel(osip_transaction_t *a, osip_message_t *q) {
    const int n = cmd_initiate_cancel(a, q);
    switch (n) {
        case 0:
            return NULL;
        case -1:
            return sip_response_new(a, q, SIP_SERVICE_UNAVAILABLE);
        default:
            return sip_response_new(a, q, SIP_INTERNAL_SERVER_ERROR);
    }
}

/**
 *
 */
static osip_message_t *sip_callback_bye(osip_transaction_t *a, osip_message_t *q) {
    const int n = cmd_initiate_bye(a, q);
    switch (n) {
        case 0:
            return NULL;
        case -1:
            return sip_response_new(a, q, SIP_SERVICE_UNAVAILABLE);
        default:
            return sip_response_new(a, q, SIP_INTERNAL_SERVER_ERROR);
    }
}

/**
 *
 */
static osip_message_t *sip_callback_info(osip_transaction_t *a, osip_message_t *q) {
    const int n = cmd_initiate_info(a, q);
    switch (n) {
        case 0:
            return NULL;
        case -1:
            return sip_response_new(a, q, SIP_SERVICE_UNAVAILABLE);
        default:
            return sip_response_new(a, q, SIP_INTERNAL_SERVER_ERROR);
    }
}

/**
 *
 */
static osip_message_t *sip_callback_subscribe(osip_transaction_t *a, osip_message_t *q) {
    const int n = cmd_initiate_subscribe(a, q);
    switch (n) {
        case 0:
            return NULL;
        case -1:
            return sip_response_new(a, q, SIP_SERVICE_UNAVAILABLE);
        default:
            return sip_response_new(a, q, SIP_INTERNAL_SERVER_ERROR);
    }
}

/**
 *
 */
static osip_message_t *sip_ack(osip_transaction_t *a, osip_message_t *q) {
    log_debug("%s: %i", __PRETTY_FUNCTION__, a->transactionid);

    return ict_create_ack(a, q);
}

/**
 *
 */
static void sip_redo(int foo, short flag, void *arg) {
    osip_transaction_t *a = arg;
    osip_event_t *e = NULL;

    const int r = osip_transaction_get_retry(a) - NULL;
    switch (a->ctx_type) {
        case ICT:
            e = __osip_event_new(r < SIP_RETRY ? TIMEOUT_A : TIMEOUT_B, a->transactionid);
            break;
        case NICT:
            e = __osip_event_new(r < SIP_RETRY ? TIMEOUT_E : TIMEOUT_F, a->transactionid);
            break;
        case IST:
            e = __osip_event_new(r < SIP_RETRY ? TIMEOUT_G : TIMEOUT_H, a->transactionid);
            break;
        default:
            log_alert(FL_WHITE "%s: [%s] NOT HANDLED IN SWITCH %s:%u" FD_NORMAL,
                      __PRETTY_FUNCTION__, sip_transaction_type(a),
                      __FILE__, __LINE__);
            break;
    }
    log_debug(FL_RED "%s: %i:%s:%i" FD_NORMAL,
              __PRETTY_FUNCTION__, a->transactionid, sip_transaction_type(a), r);

    if (e) {
        net_event_t *event = osip_transaction_get_event(a);
        if (event) {
            const int z = sip_call(a, e);
            if (z == 0) {
                osip_transaction_set_retry(a, NULL + r + 1);
            }
            net_event_update(event, NULL);
        }
    } else
        sip_delete(a);
}

/**
 *
 */
static void sip_transaction_retransmit_begin(osip_transaction_t *a) {
    /*
     * http://tools.ietf.org/html/rfc3261#section-17.1.2.2
     */
    const int r = osip_transaction_get_retry(a) - NULL;
    int t = DEFAULT_T1 * (1 << r);
    if (t > DEFAULT_T2)
        t = DEFAULT_T2;

    const net_event_t *event = osip_transaction_get_event(a);
    if (event && event->net) {
        if (event->net->protocol != IPPROTO_UDP) {
            osip_transaction_set_retry(a, NULL + 1 + SIP_RETRY);
            t = DEFAULT_T1 * 64;
        }
    }

    const struct timeval z = {.tv_sec = t / 1000, .tv_usec = 1000 * (t % 1000)};

    void *e = osip_transaction_get_timeout(a);

    net_event_timeout_begin(&e, sip_redo, a, &z);

    osip_transaction_set_timeout(a, e);
}

/**
 *
 */
static void sip_transaction_retransmit_end(osip_transaction_t *a) {
    void *e = osip_transaction_get_timeout(a);

    net_event_timeout_end(&e);

    osip_transaction_set_timeout(a, e);
}

/**
 *
 */
static osip_message_t *sip_callback_switch(const osip_message_callback_type_t type,
                                           osip_transaction_t *a, osip_message_t *q) {
    switch (type) {
        case OSIP_ICT_STATUS_TIMEOUT:
        case OSIP_ICT_ACK_SENT:
        case OSIP_ICT_ACK_SENT_AGAIN:
            break;
        case OSIP_ICT_INVITE_SENT:
        case OSIP_ICT_INVITE_SENT_AGAIN:
            sip_transaction_retransmit_begin(a);
            break;
        case OSIP_ICT_STATUS_1XX_RECEIVED:
            sip_transaction_retransmit_end(a);
            break;
        case OSIP_ICT_STATUS_2XX_RECEIVED:
        case OSIP_ICT_STATUS_2XX_RECEIVED_AGAIN:
        case OSIP_ICT_STATUS_3XX_RECEIVED:
        case OSIP_ICT_STATUS_4XX_RECEIVED:
        case OSIP_ICT_STATUS_5XX_RECEIVED:
        case OSIP_ICT_STATUS_6XX_RECEIVED:
        case OSIP_ICT_STATUS_3456XX_RECEIVED_AGAIN:
            if (MSG_IS_STATUS_2XX(q)) {
                osip_message_t *m = sip_ack(a, q);
                if (m) {
                    osip_message_replace_header(m, "Max-Forwards", SIP_DEFAULT_MAX_FORWARD);
                    sip_send(a, m, a->ict_context->destination, a->ict_context->port, 0);
                    osip_message_free(m);
                }
            }
            sip_message_remove_top_via(q);
            break;

        case OSIP_IST_ACK_RECEIVED:
        case OSIP_IST_ACK_RECEIVED_AGAIN:
            break;
        case OSIP_IST_INVITE_RECEIVED:
        case OSIP_IST_INVITE_RECEIVED_AGAIN:
            return sip_callback_invite(a, q);
        case OSIP_IST_STATUS_1XX_SENT:
            break;
        case OSIP_IST_STATUS_2XX_SENT:
        case OSIP_IST_STATUS_2XX_SENT_AGAIN:
        case OSIP_IST_STATUS_3XX_SENT:
        case OSIP_IST_STATUS_4XX_SENT:
        case OSIP_IST_STATUS_5XX_SENT:
        case OSIP_IST_STATUS_6XX_SENT:
        case OSIP_IST_STATUS_3456XX_SENT_AGAIN:
            sip_transaction_retransmit_begin(a);
            break;

        case OSIP_NIST_INFO_RECEIVED:
            return sip_callback_info(a, q);
        case OSIP_NIST_BYE_RECEIVED:
            return sip_callback_bye(a, q);
        case OSIP_NIST_CANCEL_RECEIVED:
            return sip_callback_cancel(a, q);
        case OSIP_NIST_OPTIONS_RECEIVED:
            return sip_callback_options(a, q);
        case OSIP_NIST_REGISTER_RECEIVED:
            return sip_callback_register(a, q);
        case OSIP_NIST_SUBSCRIBE_RECEIVED:
            return sip_callback_subscribe(a, q);
        case OSIP_NIST_STATUS_1XX_SENT:
        case OSIP_NIST_STATUS_2XX_SENT:
        case OSIP_NIST_STATUS_2XX_SENT_AGAIN:
        case OSIP_NIST_STATUS_3XX_SENT:
        case OSIP_NIST_STATUS_4XX_SENT:
        case OSIP_NIST_STATUS_5XX_SENT:
        case OSIP_NIST_STATUS_6XX_SENT:
        case OSIP_NIST_STATUS_3456XX_SENT_AGAIN:
            break;

        case OSIP_NICT_NOTIFY_SENT:
        case OSIP_NICT_INFO_SENT:
        case OSIP_NICT_BYE_SENT:
        case OSIP_NICT_CANCEL_SENT:
        case OSIP_NICT_OPTIONS_SENT:
        case OSIP_NICT_REQUEST_SENT_AGAIN:
            sip_transaction_retransmit_begin(a);
            break;
        case OSIP_NICT_STATUS_1XX_RECEIVED:
            sip_transaction_retransmit_end(a);
            break;
        case OSIP_NICT_STATUS_2XX_RECEIVED:
        case OSIP_NICT_STATUS_2XX_RECEIVED_AGAIN:
        case OSIP_NICT_STATUS_3XX_RECEIVED:
        case OSIP_NICT_STATUS_4XX_RECEIVED:
        case OSIP_NICT_STATUS_5XX_RECEIVED:
        case OSIP_NICT_STATUS_6XX_RECEIVED:
        case OSIP_NICT_STATUS_3456XX_RECEIVED_AGAIN:
            sip_message_remove_top_via(q);
            break;
        case OSIP_NICT_STATUS_TIMEOUT:
            break;

        default:
            log_alert(FL_WHITE "%s: [%s] NOT HANDLED IN SWITCH %s:%u" FD_NORMAL,
                      __PRETTY_FUNCTION__, sip_callback_type(type), //
                      __FILE__, __LINE__);

        case OSIP_NIST_UNKNOWN_REQUEST_RECEIVED:
        case OSIP_NIST_REQUEST_RECEIVED_AGAIN:
            if (q && IS_XIST(a) && MSG_IS_REQUEST(q))
                return sip_response_new(a, q, SIP_NOT_IMPLEMENTED);
            else
                break;
    }
    return NULL;
}

/**
 *
 */
static void sip_callback(const int type, osip_transaction_t *a, osip_message_t *q) {
    log_debug("%s: %i:%s", __PRETTY_FUNCTION__, a->transactionid, sip_callback_type(type));

    osip_message_t *m = sip_callback_switch(type, a, q);
    while (m) {
        osip_message_t *n = osip_message_get_application_data(m);
        osip_event_t *e = osip_new_outgoing_sipmessage(m);
        if (e)
            osip_transaction_add_event(a, e);
        else
            osip_message_free(m);

        m = n;
    }
}

/**
 *
 */
static void sip_transaction_kill(const int type, osip_transaction_t *a) {
    log_debug("%s: %i:%s", __PRETTY_FUNCTION__, a->transactionid, sip_callback_type(type));
}

/**
 *
 */
static void sip_transaction_fail(int type, osip_transaction_t *a, const int e) {
    log_debug("%s: %i:%s [%i]", __PRETTY_FUNCTION__, a->transactionid, sip_callback_type(type), e);
}

/**
 *
 */
int sip_init() {
    osip_trace_initialize(0,NULL);

    if (osip_init(&__sip))
        return -1;

    int mcc = OSIP_MESSAGE_CALLBACK_COUNT;
    while (mcc--)
        osip_set_message_callback(__sip, mcc, &sip_callback);

    int ncc = OSIP_KILL_CALLBACK_COUNT;
    while (ncc--)
        osip_set_kill_transaction_callback(__sip, ncc, &sip_transaction_kill);

    int ecc = OSIP_TRANSPORT_ERROR_CALLBACK_COUNT;
    while (ecc--)
        osip_set_transport_error_callback(__sip, ecc, &sip_transaction_fail);

    osip_set_cb_send_message(__sip, &sip_send);

    return 0;
}

/**
 *
 */
static void sip_destroy(void *x) {
    if (!__sip)
        return;

    osip_transaction_t *a = x;
    if (!a)
        return;

    log_debug("%s: %i", __PRETTY_FUNCTION__, a->transactionid);

    void *closure = osip_transaction_get_closure(a);
    if (closure) {
        uuid_t id;
        if (cmd_session_destroy(a->transactionid, id))
            return;
        void *opaque = osip_transaction_get_opaque(a);
        if (!a->last_response) {
            const void *retries = osip_transaction_get_retry(a);
            osip_message_t *m = sip_response_new(a, a->orig_request,
                                                 retries != NULL + SIP_RETRY
                                                     ? SIP_BAD_GATEWAY
                                                     : SIP_REQUEST_TIME_OUT);
            sip_message_remove_top_via(m);
            cmd_finalize(m, closure, opaque, id);
            osip_message_free(m);
        } else
            cmd_finalize(a->last_response, closure, opaque, id);
    }

    void *timeout = osip_transaction_get_timeout(a);
    if (timeout) {
        net_event_timeout_end(&timeout);
    }

    osip_remove_transaction(__sip, a);
    osip_transaction_free2(a);
}

/**
 * 
 * @param a
 */
void sip_delete(osip_transaction_t *a) {
    const net_event_t *net = osip_transaction_get_event(a);
    if (net)
        map_get(net->entities, a, 0);

    sip_destroy(a);
}

/**
 *
 * @param net
 * @return
 */
static int sip_timeout(net_event_t *net) {
    if (!net)
        return -1;

    const time_t now = osip_getsystemtime(NULL);
    map_t aa;
    map_init(&aa, NULL);
    map_iter_t *iter = map_iter_begin(net->entities);
    while (*iter) {
        osip_transaction_t *a = map_iter_down(net->entities, &iter);
        if (a && now - a->birth_time > 64 * DEFAULT_T1 / 1000) {
            log_debug("%s: %i/%s/%s [%i]", __PRETTY_FUNCTION__, net->event->ev_fd, net->net->name, net->app->name,
                      a->transactionid);
            map_push(&aa, a);
        }
    }
    for (osip_transaction_t *a = map_pop(&aa); a; a = map_pop(&aa))
        sip_delete(a);

    return 0;
}


/**
 *
 */
void sip_free() {
    osip_release(__sip);

    __sip = NULL;
}

static char hostname[256] = "FIXME";

const app_t __g_app_SIP = {
    "SIP",
    "0.0.0.0",
    hostname,
    "5060",
    NULL /* acquire */,
    NULL /* release */,
    sip_execute,
    sip_timeout,
    NULL,
    {{sip_destroy, map_item_cmp}},
    {8, 0}
};
