/*
 * sip.c
 *
 *  Created on: Oct 15, 2013
 *      Author: shved
 */

#include <stdlib.h>
#include <string.h>

#include "lib/proto/message_xml.h"

#include "sip.h"

/*
 * @see http://en.wikipedia.org/wiki/List_of_SIP_response_codes
 */
static const struct {
    int code;
    const char *reason;
} __reason[] = {
    {100, "Trying"},
    {180, "Ringing"},
    {181, "Call is Being Forwarded"},
    {182, "Queued"},
    {183, "Session in Progress"},
    {199, "Early Dialog Terminated"},
    {200, "OK"},
    {202, "Accepted"},
    {204, "No Notification"},
    {300, "Multiple Choices"},
    {301, "Moved Permanently"},
    {302, "Moved Temporarily"},
    {305, "Use Proxy"},
    {380, "Alternative transport"},
    {400, "Bad Request"},
    {401, "Unauthorized"},
    {402, "Payment Required"},
    {403, "Forbidden"},
    {404, "Not Found"},
    {405, "Method Not Allowed"},
    {406, "Not Acceptable"},
    {407, "Proxy Authentication Required"},
    {408, "Request Timeout"},
    {409, "Conflict"},
    {410, "Gone"},
    {411, "Length Required"},
    {412, "Conditional Request Failed"},
    {413, "Request Entity Too Large"},
    {414, "Request-URI Too Long"},
    {415, "Unsupported Media Type"},
    {416, "Unsupported URI Scheme"},
    {417, "Unknown Resource-Priority"},
    {420, "Bad Extension"},
    {421, "Extension Required"},
    {422, "Session Interval Too Small"},
    {423, "Interval Too Brief"},
    {424, "Bad Location Information"},
    {428, "Use Identity Header"},
    {429, "Provide Referrer Identity"},
    {430, "Flow Failed"},
    {433, "Anonymity Disallowed"},
    {436, "Bad Identity-Info"},
    {437, "Unsupported Certificate"},
    {438, "Invalid Identity Header"},
    {439, "First Hop Lacks Outbound Support"},
    {470, "Consent Needed"},
    {480, "Temporarily Unavailable"},
    {481, "Call/Transaction Does Not Exist"},
    {482, "Loop Detected."},
    {483, "Too Many Hops"},
    {484, "Address Incomplete"},
    {485, "Ambiguous"},
    {486, "Busy Here"},
    {487, "Request Terminated"},
    {488, "Not Acceptable Here"},
    {489, "Bad Event"},
    {491, "Request Pending"},
    {493, "Undecipherable"},
    {494, "Security Agreement Required"},
    {500, "Server Internal Error"},
    {501, "Not Implemented"},
    {502, "Bad Gateway"},
    {503, "transport Unavailable"},
    {504, "Server Timeout"},
    {505, "Version Not Supported"},
    {513, "Message Too Large"},
    {580, "Precondition Failure"},
    {600, "Busy Everywhere"},
    {603, "Decline"},
    {604, "Does Not Exist Anywhere"},
    {606, "Not Acceptable"},
    {-1, NULL}
};

char *sip_reason_by_code(int code) {
    typeof(*__reason) *m;
    for (m = __reason; m->reason; ++m)
        if (m->code == code)
            return strdup(m->reason);

    return strdup("");
}

static int __binary__proto(ProtobufCBinaryData *bin, const void *data, size_t length) {
    if (!bin)
        return 0;

    bin->len = length;

    bin->data = malloc(length);

    if (bin->len && !bin->data)
        return -1;

    memcpy(bin->data, data, length);

    return 0;
}

static int __array__proto(const osip_list_t *l, size_t *s, void ***a, void * (*call)(const void *)) {
    if (!l) {
        *s = 0;
        *a = 0;
        return 0;
    }

    *s = osip_list_size(l);
    if (!*s) {
        *a = 0;
        return 0;
    }

    *a = calloc(*s, sizeof(void *));
    if (*s && !a) {
        *s = 0;
        return -1;
    }

    int i = 0;

    for (; i < *s; ++i) {
        (*a)[i] = call(osip_list_get(l, i));
        if (!(*a)[i]) {
            *s = i;
            return -1;
        }
    }
    return 0;
}

static int __array__unproto(osip_list_t *l, const void **a, size_t n, void *(*call)(const void *)) {
    int i = 0;

    while (i < n) {
        osip_list_add(l, call(a[i]), -1);
        ++i;
    }
    return 0;
}

static int __binary__unproto(const ProtobufCBinaryData *p, void **b, size_t *n) {
    *n = p->len;
    if (!*n)
        return 0;

    *b = calloc(1, p->len + 1);
    if (*n && !*b)
        return -1;

    memcpy(*b, p->data, p->len);

    return 0;
}

#define array__unproto(__l, __p, __f)	__array__unproto((__l), (const void **) (__p)->__f, (__p)->n_##__f, (void * (*)(const void *)) &sip__type__##__f##__unproto)
#define array__proto(__l, __x, __a) 	__array__proto((__l), &((__x)->n_##__a), (void ***) &((__x)->__a), (void * (*)(const void *)) &sip__type__##__a##__proto)
#define binary__proto(__b, __m, __l) 	__binary__proto(__b, __m, __l)
#define binary__unproto(__b, __m, __l) 	__binary__unproto(__b, (void **)__m, __l)
#define strdup_null(__s)				__s ? osip_strdup(__s) : NULL
#define strdup_null_without_quote(__s)	__s ? osip_strdup_without_quote(__s) : NULL
#define strdup_null_with_quote(__s)		__s ? osip_enquote(__s) : NULL

char *sip__type__allow__proto(const osip_allow_t *q) {
    if (!q)
        return NULL;

    return strdup_null_without_quote(q->value);
}

Sip__Type__CallId *sip__type__call_id__new(const char *number, const char *host) {
    Sip__Type__CallId *p = malloc(sizeof(Sip__Type__CallId));
    if (p) {
        sip__type__call_id__init(p);
        p->host = strdup_null(host);
        p->number = strdup_null(number);
    }
    return p;
}

Sip__Type__CallId *sip__type__call_id__proto(const osip_call_id_t *q) {
    if (!q)
        return NULL;
    else
        return sip__type__call_id__new(q->number, q->host);
}

Sip__Type__Cseq *sip__type__cseq__new(const char *number, const char *method) {
    Sip__Type__Cseq *p = malloc(sizeof(Sip__Type__Cseq));
    if (p) {
        sip__type__cseq__init(p);
        p->method = strdup_null(method);
        p->number = strdup_null(number);
    }
    return p;
}

Sip__Type__Cseq *sip__type__cseq__proto(const osip_cseq_t *q) {
    if (!q)
        return NULL;
    else
        return sip__type__cseq__new(q->number, q->method);
}

Sip__Type__Pair *sip__type__pair__new(const char *name, const char *value) {
    Sip__Type__Pair *p = malloc(sizeof(Sip__Type__Pair));
    if (p) {
        sip__type__pair__init(p);
        p->name = strdup_null(name);
        p->value = strdup_null(value);
    }
    return p;
}

Sip__Type__Pair *sip__type__pair__proto(const osip_generic_param_t *q) {
    if (!q)
        return NULL;
    else
        return sip__type__pair__new(q->gname, q->gvalue);
}

Sip__Type__Uri *sip__type__uri__new(const char *scheme, const char *username, const char *password,
                                    const char *host, const char *port, const char *other) {
    Sip__Type__Uri *p = malloc(sizeof(Sip__Type__Uri));
    while (p) {
        sip__type__uri__init(p);
        p->scheme = strdup_null(scheme);
        if (scheme && !p->scheme)
            break;
        p->username = strdup_null(username);
        if (username && !p->username)
            break;
        p->password = strdup_null(password);
        if (password && !p->password)
            break;
        p->host = strdup_null(host);
        if (host && !p->host)
            break;
        p->port = strdup_null(port);
        if (port && !p->port)
            break;
        p->other = strdup_null(other);
        if (other && !p->other)
            break;
        return p;
    }
    if (p)
        sip__type__uri__free_unpacked(p, 0);
    return NULL;
}

Sip__Type__Uri *sip__type__uri__proto(const osip_uri_t *q) {
    if (!q)
        return NULL;

    Sip__Type__Uri *p = sip__type__uri__new(q->scheme, q->username, q->password, q->host, q->port, q->string);
    while (p) {
        if (array__proto(&q->url_headers, p, url_headers))
            break;
        if (array__proto(&q->url_params, p, url_params))
            break;
        return p;
    }
    if (p)
        sip__type__uri__free_unpacked(p, 0);
    return NULL;
}

Sip__Type__Address *sip__type__address__proto(const osip_from_t *q) {
    if (!q)
        return NULL;

    Sip__Type__Address *p = malloc(sizeof(Sip__Type__Address));
    while (p) {
        sip__type__address__init(p);
        p->displayname = strdup_null_without_quote(q->displayname);
        p->url = sip__type__uri__proto(q->url);
        if (array__proto(&q->gen_params, p, gen_params))
            break;
        return p;
    }
    if (p)
        sip__type__address__free_unpacked(p, 0);
    return NULL;
}

Sip__Type__ContentType *sip__type__content_type__proto(const osip_content_type_t *q) {
    if (!q)
        return NULL;

    Sip__Type__ContentType *p = malloc(sizeof(Sip__Type__ContentType));
    while (p) {
        sip__type__content_type__init(p);
        p->type = strdup_null(q->type);
        if (q->type && !p->type)
            break;
        p->subtype = strdup_null(q->subtype);
        if (q->subtype && !p->subtype)
            break;
        if (array__proto(&q->gen_params, p, gen_params))
            break;
        return p;
    }
    if (p)
        sip__type__content_type__free_unpacked(p, 0);
    return NULL;
}

Sip__Type__Content *sip__type__content__proto(const osip_body_t *q) {
    if (!q)
        return NULL;

    Sip__Type__Content *p = malloc(sizeof(Sip__Type__Content));
    while (p) {
        sip__type__content__init(p);
        p->content_type = sip__type__content_type__proto(q->content_type);
        if (q->content_type && !p->content_type)
            break;
        if (array__proto(q->headers, p, headers))
            break;
        if (binary__proto(&p->body, q->body, q->length))
            break;
        return p;
    }
    if (p)
        sip__type__content__free_unpacked(p, 0);
    return NULL;
}

Sip__Type__Authorization *sip__type__authorization__proto(const osip_authorization_t *q) {
    if (!q)
        return NULL;

    Sip__Type__Authorization *p = malloc(sizeof(Sip__Type__Authorization));
    if (p) {
        sip__type__authorization__init(p);
        p->algorithm = strdup_null_without_quote(q->algorithm);
        p->auth_param = strdup_null_without_quote(q->auth_param);
        p->auth_type = strdup_null_without_quote(q->auth_type);
        p->cnonce = strdup_null_without_quote(q->cnonce);
        p->digest = strdup_null_without_quote(q->digest);
        p->message_qop = strdup_null_without_quote(q->message_qop);
        p->nonce = strdup_null_without_quote(q->nonce);
        p->nonce_count = strdup_null_without_quote(q->nonce_count);
        p->opaque = strdup_null_without_quote(q->opaque);
        p->realm = strdup_null_without_quote(q->realm);
        p->response = strdup_null_without_quote(q->response);
        p->uri = strdup_null_without_quote(q->uri);
        p->username = strdup_null_without_quote(q->username);
    }
    return p;
}

Sip__Type__Authenticate *sip__type__authenticate__new(const char *algorithm, const char *auth_param,
                                                      const char *auth_type, const char *domain, const char *nonce,
                                                      const char *opaque,
                                                      const char *qop_options, const char *realm, const char *stale) {
    Sip__Type__Authenticate *p = malloc(sizeof(Sip__Type__Authenticate));
    if (p) {
        sip__type__authenticate__init(p);
        p->algorithm = strdup_null_without_quote(algorithm);
        p->auth_param = strdup_null_without_quote(auth_param);
        p->auth_type = strdup_null_without_quote(auth_type);
        p->domain = strdup_null_without_quote(domain);
        p->nonce = strdup_null_without_quote(nonce);
        p->opaque = strdup_null_without_quote(opaque);
        p->qop_options = strdup_null_without_quote(qop_options);
        p->realm = strdup_null_without_quote(realm);
        p->stale = strdup_null_without_quote(stale);
    }
    return p;
}

Sip__Type__Authenticate *sip__type__authenticate__proto(const osip_www_authenticate_t *q) {
    if (!q)
        return NULL;
    else
        return sip__type__authenticate__new(q->algorithm, q->auth_param, q->auth_type, q->domain, q->nonce,
                                            q->opaque, q->qop_options, q->realm, q->stale);
}

Sip__Type__Authentication *sip__type__authentication__proto(const osip_authentication_info_t *q) {
    if (!q)
        return NULL;

    Sip__Type__Authentication *p = malloc(sizeof(Sip__Type__Authenticate));
    if (p) {
        sip__type__authentication__init(p);
        p->cnonce = strdup_null_without_quote(q->cnonce);
        p->nextnonce = strdup_null_without_quote(q->nextnonce);
        p->nonce_count = strdup_null_without_quote(q->nonce_count);
        p->qop_options = strdup_null_without_quote(q->qop_options);
        p->rspauth = strdup_null_without_quote(q->rspauth);
    }
    return p;
}

Sip__Type__Encode *sip__type__encode__proto(const osip_accept_encoding_t *q) {
    if (!q)
        return NULL;

    Sip__Type__Encode *p = malloc(sizeof(Sip__Type__Encode));
    while (p) {
        sip__type__encode__init(p);
        p->element = strdup_null(q->element);
        if (array__proto(&q->gen_params, p, gen_params))
            break;
        return p;
    }
    if (p)
        sip__type__encode__free_unpacked(p, 0);
    return NULL;
}

Sip__Type__Via *sip__type__via__proto(const osip_via_t *q) {
    if (!q)
        return NULL;

    Sip__Type__Via *p = malloc(sizeof(Sip__Type__Via));
    while (p) {
        sip__type__via__init(p);
        p->host = strdup_null(q->host);
        p->port = strdup_null(q->port);
        p->protocol = strdup_null(q->protocol);
        p->version = strdup_null(q->version);
        p->comment = strdup_null(q->comment);
        if (array__proto(&q->via_params, p, via_params))
            break;
        return p;
    }
    if (p)
        sip__type__via__free_unpacked(p, 0);
    return NULL;
}

static Sip__Head *sip__head__proto(const osip_message_t *m) {
    if (!m)
        return NULL;

    Sip__Head *p = malloc(sizeof(Sip__Head));
    while (p) {
        sip__head__init(p);
        p->version = strdup_null(m->sip_version);
        p->call_id = sip__type__call_id__proto(m->call_id);
        p->cseq = sip__type__cseq__proto(m->cseq);
        p->from = sip__type__address__proto(m->from);
        p->to = sip__type__address__proto(m->to);
        p->content_type = sip__type__content_type__proto(m->content_type);

        if (array__proto(&m->accepts, p, accept))
            break;
        if (array__proto(&m->accept_encodings, p, accept_encoding))
            break;
        if (array__proto(&m->accept_languages, p, accept_language))
            break;
        if (array__proto(&m->alert_infos, p, alert_info))
            break;
        if (array__proto(&m->call_infos, p, call_info))
            break;
        if (array__proto(&m->error_infos, p, error_info))
            break;
        if (array__proto(&m->content_encodings, p, content_encoding))
            break;
        if (array__proto(&m->allows, p, allow))
            break;
        if (array__proto(&m->contacts, p, contact))
            break;
        if (array__proto(&m->proxy_authenticates, p, proxy_authenticate))
            break;
        if (array__proto(&m->proxy_authentication_infos, p, proxy_authentication))
            break;
        if (array__proto(&m->proxy_authorizations, p, proxy_authorization))
            break;
        if (array__proto(&m->www_authenticates, p, authenticate))
            break;
        if (array__proto(&m->authentication_infos, p, authentication))
            break;
        if (array__proto(&m->authorizations, p, authorization))
            break;
        if (array__proto(&m->record_routes, p, record_route))
            break;
        if (array__proto(&m->routes, p, route))
            break;
        if (array__proto(&m->vias, p, via))
            break;
        if (array__proto(&m->headers, p, other))
            break;
        return p;
    }
    if (p)
        sip__head__free_unpacked(p, 0);
    return NULL;
}

static int __content__equal(osip_content_type_t *content, const char *type) {
    const char *subtype = strchr(type, '/');
    if (!subtype)
        return 0;

    int n = subtype - type;
    if (!n)
        return 0;

    if (content && content->type && content->subtype)
        return strncasecmp(content->type, type, n) == 0 && content->type[n] == 0
                   ? strcasecmp(content->subtype, subtype + 1) == 0
                         ? 1
                         : 0
                   : 0;

    return 0;
}

Sip__Type__Vfu *sip__type__vfu__proto(xml_t *q) {
    if (!q)
        return NULL;

    Sip__Type__Vfu *p = calloc(1, sizeof(Sip__Type__Vfu));
    if (p) {
        sip__type__vfu__init(p);
        protobuf_c_message_from_xml(&p->base, &q->document);
    }
    return p;
}

Sip__Type__Pidf *sip__type__pidf__proto(xml_t *q) {
    if (!q)
        return NULL;

    Sip__Type__Pidf *p = calloc(1, sizeof(Sip__Type__Pidf));
    if (p) {
        sip__type__pidf__init(p);
        protobuf_c_message_from_xml(&p->base, &q->document);
    }
    return p;
}

Sip__Type__Rl *sip__type__rl__proto(xml_t *q) {
    if (!q)
        return NULL;

    Sip__Type__Rl *p = calloc(1, sizeof(Sip__Type__Rl));
    if (p) {
        sip__type__rl__init(p);
        protobuf_c_message_from_xml(&p->base, &q->document);
    }
    return p;
}

Sip__Type__Dtmf *sip__type__dtmf__proto(const char *q) {
    if (!q)
        return NULL;

    unsigned duration;
    char key[4];

    // http://tools.ietf.org/html/draft-kaplan-dispatch-info-dtmf-package-00#section-5
    // http://tools.ietf.org/html/rfc2833#section-3.10

    int n = sscanf(q, "Signal = %3[0-9A-D*#] Duration = %u", key, &duration);
    if (n != 2)
        return NULL;

    Sip__Type__Dtmf *p = calloc(1, sizeof(Sip__Type__Dtmf));
    if (p) {
        sip__type__dtmf__init(p);
        switch (*key) {
            case '*':
                p->signal = 0xA;
                break;
            case '#':
                p->signal = 0xB;
                break;
            case 'A':
            case 'B':
            case 'C':
            case 'D':
                p->signal = strtoul(key, NULL, 16) + 2;
                break;
            default:
                p->signal = strtoul(key, NULL, 10);
                break;
        }
        p->duration = duration;
    }
    return p;
}

static int __content__proto(const osip_message_t *m, Sip__Type__Content ***p_content, size_t *n_content,
                            Sip__Type__Sdp **p_sdp, Sip__Type__Vfu **p_vfu, Sip__Type__Dtmf **p_dtmf,
                            Sip__Type__Pidf **p_pidf,
                            Sip__Type__Rl **p_rl) {
    if (!m)
        return -1;

    osip_list_t content;
    osip_list_init(&content);

    int n = osip_list_size(&m->bodies);
    while (n--) {
        osip_body_t *body = osip_list_get(&m->bodies, n);
        if (body) {
#ifdef SIP__TYPE__CONTENT_TYPE__SDP
            if (__content__equal(body->content_type ? body->content_type : m->content_type,
                                 SIP__TYPE__CONTENT_TYPE__SDP)) {
                if (p_sdp && *p_sdp == NULL) {
                    sdp_message_t *sdp = NULL;
                    sdp_message_init(&sdp);
                    sdp_message_parse(sdp, body->body);
                    *p_sdp = sip__type__sdp__proto(sdp);
                    sdp_message_free(sdp);
                    if (*p_sdp)
                        continue;
                }
            }
#endif
#ifdef SIP__TYPE__CONTENT_TYPE__MEDIA_CONTROL
            if (__content__equal(body->content_type ? body->content_type : m->content_type,
                                 SIP__TYPE__CONTENT_TYPE__MEDIA_CONTROL)) {
                if (p_vfu && *p_vfu == NULL) {
                    xml_t xml = xml_INITIALIZER;
                    xml_init(&xml);
                    xml_feed(&xml, body->body, body->length);
                    *p_vfu = sip__type__vfu__proto(&xml);
                    xml_free(&xml);
                    if (*p_vfu)
                        continue;
                }
            }
#endif
#ifdef SIP__TYPE__CONTENT_TYPE__DTMF_RELAY
            if (__content__equal(body->content_type ? body->content_type : m->content_type,
                                 SIP__TYPE__CONTENT_TYPE__DTMF_RELAY)) {
                if (p_dtmf && *p_dtmf == NULL) {
                    *p_dtmf = sip__type__dtmf__proto(body->body);
                    if (*p_dtmf)
                        continue;
                }
            }
#endif
#ifdef SIP__TYPE__CONTENT_TYPE__PIDF
            if (__content__equal(body->content_type ? body->content_type : m->content_type,
                                 SIP__TYPE__CONTENT_TYPE__PIDF)) {
                if (p_pidf && *p_pidf == NULL) {
                    xml_t xml = xml_INITIALIZER;
                    xml_init(&xml);
                    xml_feed(&xml, body->body, body->length);
                    *p_pidf = sip__type__pidf__proto(&xml);
                    xml_free(&xml);
                    if (*p_pidf)
                        continue;
                }
            }
#endif
#ifdef SIP__TYPE__CONTENT_TYPE__RESOURCE_LISTS
            if (__content__equal(body->content_type ? body->content_type : m->content_type,
                                 SIP__TYPE__CONTENT_TYPE__RESOURCE_LISTS)) {
                if (p_rl && *p_rl == NULL) {
                    xml_t xml = xml_INITIALIZER;
                    xml_init(&xml);
                    xml_feed(&xml, body->body, body->length);
                    *p_rl = sip__type__rl__proto(&xml);
                    xml_free(&xml);
                    if (*p_rl)
                        continue;
                }
            }
#endif
            osip_list_add(&content, body, 0);
        }
    }

    int z = __array__proto(&content, n_content, (void ***) p_content,
                           (void * (*)(const void *)) &sip__type__content__proto);

    osip_list_special_free(&content, NULL);

    return z;
}

Sip__Query *sip__query__proto(const osip_message_t *m) {
    if (!m)
        return NULL;

    Sip__Query *p = malloc(sizeof(Sip__Query));
    while (p) {
        sip__query__init(p);
        p->request = sip__type__uri__proto(m->req_uri);
        if (m->req_uri && !p->request)
            break;
        p->head = sip__head__proto(m);
        if (!p->head)
            break;

        if (__content__proto(m, &p->content, &p->n_content, &p->sdp, &p->vfu, &p->dtmf, &p->pidf, &p->rl))
            break;

        return p;
    }
    if (p)
        sip__query__free_unpacked(p, 0);
    return NULL;
}

Sip__Answer *sip__answer__proto(const osip_message_t *m) {
    if (!m)
        return NULL;

    Sip__Answer *p = malloc(sizeof(Sip__Answer));
    while (p) {
        sip__answer__init(p);
        p->response = m->status_code;
        p->head = sip__head__proto(m);
        if (!p->head)
            break;

        if (__content__proto(m, &p->content, &p->n_content, &p->sdp, NULL, NULL, NULL, &p->rl))
            break;

        return p;
    }
    if (p)
        sip__answer__free_unpacked(p, 0);
    return NULL;
}

osip_allow_t *sip__type__allow__unproto(const char *p) {
    if (!p)
        return NULL;

    osip_content_length_t *q = NULL;
    osip_content_length_init(&q);

    q->value = strdup_null_without_quote(p);

    return q;
}

osip_generic_param_t *sip__type__pair__unproto(const Sip__Type__Pair *p) {
    osip_uri_param_t *q = NULL;
    osip_uri_param_init(&q);

    q->gname = strdup_null(p->name);
    q->gvalue = strdup_null(p->value);

    return q;
}

osip_uri_t *sip__type__uri__unproto(const Sip__Type__Uri *p) {
    if (!p)
        return NULL;

    osip_uri_t *q = NULL;
    osip_uri_init(&q);

    q->host = strdup_null(p->host);
    q->password = strdup_null(p->password);
    q->port = strdup_null(p->port);
    q->scheme = strdup_null(p->scheme);
    q->string = strdup_null(p->other);
    q->username = strdup_null(p->username);

    array__unproto(&q->url_headers, p, url_headers);
    array__unproto(&q->url_params, p, url_params);

    return q;
}

osip_call_id_t *sip__type__call_id__unproto(const Sip__Type__CallId *p) {
    if (!p)
        return NULL;

    osip_call_id_t *q = NULL;
    osip_call_id_init(&q);

    q->host = strdup_null(p->host);
    q->number = strdup_null(p->number);

    return q;
}

osip_cseq_t *sip__type__cseq__unproto(const Sip__Type__Cseq *p) {
    if (!p)
        return NULL;

    osip_cseq_t *q = NULL;
    osip_cseq_init(&q);

    q->method = strdup_null(p->method);
    q->number = strdup_null(p->number);

    return q;
}

osip_from_t *sip__type__address__unproto(const Sip__Type__Address *p) {
    if (!p)
        return NULL;

    osip_from_t *q = NULL;
    osip_from_init(&q);

    q->displayname = strdup_null_with_quote(p->displayname);
    q->url = sip__type__uri__unproto(p->url);

    array__unproto(&q->gen_params, p, gen_params);

    return q;
}

osip_content_type_t *sip__type__content_type__unproto(const Sip__Type__ContentType *p) {
    if (!p)
        return NULL;

    osip_content_type_t *q = NULL;
    osip_content_type_init(&q);

    q->type = strdup_null(p->type);
    q->subtype = strdup_null(p->subtype);

    array__unproto(&q->gen_params, p, gen_params);

    return q;
}

osip_accept_encoding_t *sip__type__encode__unproto(const Sip__Type__Encode *p) {
    if (!p)
        return NULL;

    osip_accept_encoding_t *q = NULL;
    osip_accept_encoding_init(&q);

    q->element = strdup_null(p->element);

    array__unproto(&q->gen_params, p, gen_params);

    return q;
}

osip_body_t *sip__type__content__unproto(const Sip__Type__Content *p) {
    if (!p)
        return NULL;

    osip_body_t *b = NULL;
    osip_body_init(&b);

    binary__unproto(&p->body, &b->body, &b->length);

    return b;
}

osip_authorization_t *sip__type__authorization__unproto(const Sip__Type__Authorization *p) {
    if (!p)
        return NULL;

    osip_authorization_t *q = NULL;
    osip_authorization_init(&q);

    q->algorithm = strdup_null(p->algorithm);
    q->auth_param = strdup_null(p->auth_param);
    q->auth_type = strdup_null(p->auth_type);
    q->cnonce = strdup_null_with_quote(p->cnonce);
    q->digest = strdup_null(p->digest);
    q->message_qop = strdup_null_with_quote(p->message_qop);
    q->nonce = strdup_null_with_quote(p->nonce);
    q->nonce_count = strdup_null(p->nonce_count);
    q->opaque = strdup_null_with_quote(p->opaque);
    q->realm = strdup_null_with_quote(p->realm);
    q->response = strdup_null_with_quote(p->response);
    q->uri = strdup_null_with_quote(p->uri);
    q->username = strdup_null_with_quote(p->username);

    return q;
}

osip_www_authenticate_t *sip__type__authenticate__unproto(const Sip__Type__Authenticate *p) {
    if (!p)
        return NULL;

    osip_www_authenticate_t *q = NULL;
    osip_www_authenticate_init(&q);

    q->algorithm = strdup_null(p->algorithm);
    q->auth_param = strdup_null(p->auth_param);
    q->auth_type = strdup_null(p->auth_type);
    q->domain = strdup_null_with_quote(p->domain);
    q->nonce = strdup_null_with_quote(p->nonce);
    q->opaque = strdup_null_with_quote(p->opaque);
    q->qop_options = strdup_null_with_quote(p->qop_options);
    q->realm = strdup_null_with_quote(p->realm);
    q->stale = strdup_null(p->stale);

    return q;
}

osip_authentication_info_t *sip__type__authentication__unproto(const Sip__Type__Authentication *p) {
    if (!p)
        return NULL;

    osip_authentication_info_t *q = NULL;
    osip_authentication_info_init(&q);

    q->cnonce = strdup_null_with_quote(p->cnonce);
    q->nextnonce = strdup_null_with_quote(p->nextnonce);
    q->nonce_count = strdup_null(p->nonce_count);
    q->qop_options = strdup_null_with_quote(p->qop_options);
    q->rspauth = strdup_null_with_quote(p->rspauth);

    return q;
}

osip_via_t *sip__type__via__unproto(const Sip__Type__Via *p) {
    if (!p)
        return NULL;

    osip_via_t *q = NULL;
    osip_via_init(&q);

    q->comment = strdup_null(p->comment);
    q->host = strdup_null(p->host);
    q->port = strdup_null(p->port);
    q->protocol = strdup_null(p->protocol);
    q->version = strdup_null(p->version);

    array__unproto(&q->via_params, p, via_params);

    return q;
}

static osip_message_t *sip__head__unproto(osip_message_t *m, const Sip__Head *p, unsigned bitset) {
    if (!m || !p)
        return NULL;

    if (bitset & VERSION_BIT)
        m->sip_version = strdup_null(p->version);
    if (bitset & CALLID_BIT)
        m->call_id = sip__type__call_id__unproto(p->call_id);
    if (bitset & CSEQ_BIT)
        m->cseq = sip__type__cseq__unproto(p->cseq);
    if (bitset & FROM_BIT)
        m->from = sip__type__address__unproto(p->from);
    if (bitset & TO_BIT)
        m->to = sip__type__address__unproto(p->to);
    if (bitset & CONTENT_TYPE_BIT)
        m->content_type = sip__type__content_type__unproto(p->content_type);

    if (bitset & ACCEPT_BIT)
        array__unproto(&m->accepts, p, accept);
    if (bitset & ACCEPT_ENCODING_BIT)
        array__unproto(&m->accept_encodings, p, accept_encoding);
    if (bitset & ACCEPT_LANGUAGE_BIT)
        array__unproto(&m->accept_languages, p, accept_language);
    if (bitset & ALERT_INFO_BIT)
        array__unproto(&m->alert_infos, p, alert_info);
    if (bitset & CALL_INFO_BIT)
        array__unproto(&m->call_infos, p, call_info);
    if (bitset & ERROR_INFO_BIT)
        array__unproto(&m->error_infos, p, error_info);
    if (bitset & CONTENT_ENCODING_BIT)
        array__unproto(&m->content_encodings, p, content_encoding);
    if (bitset & ALLOW_BIT)
        array__unproto(&m->allows, p, allow);
    if (bitset & CONTACT_BIT)
        array__unproto(&m->contacts, p, contact);
    if (bitset & PROXY_AUTHENTICATE_BIT)
        array__unproto(&m->proxy_authenticates, p, proxy_authenticate);
    if (bitset & PROXY_AUTHENTICATION_BIT)
        array__unproto(&m->proxy_authentication_infos, p, proxy_authentication);
    if (bitset & PROXY_AUTHORIZATION_BIT)
        array__unproto(&m->proxy_authorizations, p, proxy_authorization);
    if (bitset & AUTHENTICATE_BIT)
        array__unproto(&m->www_authenticates, p, authenticate);
    if (bitset & AUTHENTICATION_BIT)
        array__unproto(&m->authentication_infos, p, authentication);
    if (bitset & AUTHORIZATION_BIT)
        array__unproto(&m->authorizations, p, authorization);
    if (bitset & RECORD_ROUTE_BIT)
        array__unproto(&m->record_routes, p, record_route);
    if (bitset & ROUTE_BIT)
        array__unproto(&m->routes, p, route);
    if (bitset & VIA_BIT)
        array__unproto(&m->vias, p, via);
    if (bitset & OTHER_BIT)
        array__unproto(&m->headers, p, other);

    return m;
}

static int __content__unproto(osip_message_t *m, Sip__Type__Content **const p_content, size_t n_content,
                              const Sip__Type__Sdp *p_sdp, const Sip__Type__Vfu *p_vfu, const Sip__Type__Dtmf *p_dtmf,
                              Sip__Type__Pidf *p_pidf, Sip__Type__Rl *p_rl) {
    if (!m)
        return -1;

#ifdef SIP__TYPE__CONTENT_TYPE__SDP
    if (p_sdp != NULL) {
        sdp_message_t *sdp = sip__type__sdp__unproto(p_sdp);
        if (sdp) {
            osip_body_t *body = NULL;
            osip_body_init(&body);
            if (n_content || osip_list_size(&m->bodies))
                osip_body_set_contenttype(body, SIP__TYPE__CONTENT_TYPE__SDP);
            int n = sdp_message_to_str(sdp, &body->body);
            if (n == 0 && body->body && osip_list_add(&m->bodies, body, 0) > 0)
                body->length = strlen(body->body);
            else
                osip_body_free(body);
            sdp_message_free(sdp);
        }
    }
#endif
#ifdef SIP__TYPE__CONTENT_TYPE__MEDIA_CONTROL
    if (p_vfu != NULL) {
        char buff[BODY_MESSAGE_MAX_SIZE];
        xml_t xml = xml_INITIALIZER;
        xml_init(&xml);
        protobuf_c_message_to_xml(&p_vfu->base, &xml.document);
        osip_body_t *body = NULL;
        osip_body_init(&body);
        if (n_content || osip_list_size(&m->bodies))
            osip_body_set_contenttype(body, SIP__TYPE__CONTENT_TYPE__MEDIA_CONTROL);
        int n = xml_seed(&xml, buff, BODY_MESSAGE_MAX_SIZE - 1, '"');
        if (n > 0 && osip_list_add(&m->bodies, body, 0) > 0) {
            buff[n] = 0;
            body->body = osip_strdup(buff);
            body->length = n;
        } else
            osip_body_free(body);
        xml_free(&xml);
    }
#endif
#ifdef SIP__TYPE__CONTENT_TYPE__DTMF_RELAY
    if (p_dtmf != NULL) {
        char dtmf[BODY_MESSAGE_MAX_SIZE];
        osip_body_t *body = NULL;
        osip_body_init(&body);
        if (n_content || osip_list_size(&m->bodies))
            osip_body_set_contenttype(body, SIP__TYPE__CONTENT_TYPE__DTMF_RELAY);
        const int n = sprintf(dtmf, "Signal=%c%sDuration=%u",
                        p_dtmf->signal < 0xA
                            ? '0' + p_dtmf->signal
                            : p_dtmf->signal == 10
                                  ? '*'
                                  : p_dtmf->signal == 11
                                        ? '#'
                                        : p_dtmf->signal < 0x10
                                              ? 'A' + p_dtmf->signal - 12
                                              : 0, CRLF,
                        p_dtmf->duration);
        if (n > 0 && osip_list_add(&m->bodies, body, 0) > 0) {
            body->body = osip_strdup(dtmf);
            body->length = n;
        } else
            osip_body_free(body);
    }
#endif
#ifdef SIP__TYPE__CONTENT_TYPE__PIDF
    if (p_pidf != NULL) {
        char buff[BODY_MESSAGE_MAX_SIZE];
        xml_t xml = xml_INITIALIZER;
        xml_init(&xml);
        protobuf_c_message_to_xml(&p_pidf->base, &xml.document);
        osip_body_t *body = NULL;
        osip_body_init(&body);
        if (n_content || osip_list_size(&m->bodies))
            osip_body_set_contenttype(body, SIP__TYPE__CONTENT_TYPE__PIDF);
        const int n = xml_seed(&xml, buff, BODY_MESSAGE_MAX_SIZE - 1, '"');
        if (n > 0 && osip_list_add(&m->bodies, body, 0) > 0) {
            buff[n] = 0;
            body->body = osip_strdup(buff);
            body->length = n;
        } else
            osip_body_free(body);
        xml_free(&xml);
    }
#endif
#ifdef SIP__TYPE__CONTENT_TYPE__RESOURCE_LISTS
    if (p_rl != NULL) {
        char buff[BODY_MESSAGE_MAX_SIZE];
        xml_t xml = xml_INITIALIZER;
        xml_init(&xml);
        protobuf_c_message_to_xml(&p_rl->base, &xml.document);
        osip_body_t *body = NULL;
        osip_body_init(&body);
        if (n_content || osip_list_size(&m->bodies))
            osip_body_set_contenttype(body, SIP__TYPE__CONTENT_TYPE__RESOURCE_LISTS);
        int n = xml_seed(&xml, buff, BODY_MESSAGE_MAX_SIZE - 1, '"');
        if (n > 0 && osip_list_add(&m->bodies, body, 0) > 0) {
            buff[n] = 0;
            body->body = osip_strdup(buff);
            body->length = n;
        } else
            osip_body_free(body);
        xml_free(&xml);
    }
#endif

    return __array__unproto(&m->bodies, (const void **) p_content, n_content,
                            (void *(*)(const void *)) &sip__type__content__unproto);
}

osip_message_t *sip__query__unproto(const Sip__Query *p, const char *method, const unsigned bitset) {
    if (!p)
        return NULL;

    osip_message_t *m = NULL;
    osip_message_init(&m);

    m->sip_method = strdup_null(method);
    m->req_uri = sip__type__uri__unproto(p->request);

    __content__unproto(m, p->content, p->n_content, p->sdp, p->vfu, p->dtmf, p->pidf, p->rl);

    // FIXME
    // if (osip_list_size(&m->bodies) > 1) {
    //     osip_content_length_init(&m->mime_version);
    //     if (m->mime_version)
    //         m->mime_version->value = strdup("1.0");
    // }

    return sip__head__unproto(m, p->head, bitset);
}

osip_message_t *sip__answer__unproto(const Sip__Answer *p, const unsigned bitset) {
    if (!p)
        return NULL;

    osip_message_t *m = NULL;
    osip_message_init(&m);

    m->status_code = p->response;
    m->reason_phrase = sip_reason_by_code(m->status_code);

    __content__unproto(m, p->content, p->n_content, p->sdp, NULL, NULL, NULL, p->rl);

    // FIXME
    // if (osip_list_size(&m->bodies) > 1) {
    // 	osip_content_length_init(&m->mime_version);
    // 	if (m->mime_version)
    // 		m->mime_version->value = strdup("1.0");
    // }

    return sip__head__unproto(m, p->head, bitset);
}

#define sip__type__char__unproto sip__type__char__proto

char *sip__type__char__proto(const char *q) {
    return strdup_null(q);
}

Sip__Type__Sdp__Connection *sip__type__sdp__connection__new(const char *c_addr, const char *c_addrtype,
                                                            const char *c_nettype, const char *c_addr_multicast_int,
                                                            const char *c_addr_multicast_ttl) {
    Sip__Type__Sdp__Connection *p = calloc(1, sizeof(Sip__Type__Sdp__Connection));
    if (p) {
        sip__type__sdp__connection__init(p);
        p->c_addr = strdup_null(c_addr);
        p->c_addr_multicast_int = strdup_null(c_addr_multicast_int);
        p->c_addr_multicast_ttl = strdup_null(c_addr_multicast_ttl);
        p->c_addrtype = strdup_null(c_addrtype);
        p->c_nettype = strdup_null(c_nettype);
    }
    return p;
}

Sip__Type__Sdp__Connection *sip__type__sdp__connection__proto(const sdp_connection_t *q) {
    if (!q)
        return NULL;
    else
        return sip__type__sdp__connection__new(q->c_addr, q->c_addrtype, q->c_nettype,
                                               q->c_addr_multicast_int, q->c_addr_multicast_ttl);
}

Sip__Type__Sdp__Bandwidth *sip__type__sdp__bandwidth__new(const char *b_bandwidth, const char *b_bwtype) {
    Sip__Type__Sdp__Bandwidth *p = calloc(1, sizeof(Sip__Type__Sdp__Bandwidth));
    if (p) {
        sip__type__sdp__bandwidth__init(p);
        p->b_bandwidth = strdup_null(b_bandwidth);
        p->b_bwtype = strdup_null(b_bwtype);
    }
    return p;
}

Sip__Type__Sdp__Bandwidth *sip__type__sdp__bandwidth__proto(const sdp_bandwidth_t *q) {
    if (!q)
        return NULL;
    else
        return sip__type__sdp__bandwidth__new(q->b_bandwidth, q->b_bwtype);
}

Sip__Type__Sdp__Time *sip__type__sdp__time__new(const char *t_start_time, const char *t_stop_time) {
    Sip__Type__Sdp__Time *p = calloc(1, sizeof(Sip__Type__Sdp__Time));
    if (p) {
        sip__type__sdp__time__init(p);
        p->t_start_time = strdup_null(t_start_time);
        p->t_stop_time = strdup_null(t_stop_time);
    }
    return p;
}

Sip__Type__Sdp__Time *sip__type__sdp__time__proto(const sdp_time_descr_t *q) {
    if (!q)
        return NULL;

    Sip__Type__Sdp__Time *p = sip__type__sdp__time__new(q->t_start_time, q->t_stop_time);
    if (p) {
        array__proto(&q->r_repeats, p, r_repeats);
    }
    return p;
}

Sip__Type__Sdp__Key *sip__type__sdp__key__new(const char *k_keydata, const char *k_keytype) {
    Sip__Type__Sdp__Key *p = calloc(1, sizeof(Sip__Type__Sdp__Key));
    if (p) {
        sip__type__sdp__key__init(p);
        p->k_keydata = strdup_null(k_keydata);
        p->k_keytype = strdup_null(k_keytype);
    }
    return p;
}

Sip__Type__Sdp__Key *sip__type__sdp__key__proto(const sdp_key_t *q) {
    if (!q)
        return NULL;
    else
        return sip__type__sdp__key__new(q->k_keydata, q->k_keytype);
}

Sip__Type__Sdp__Attribute *sip__type__sdp__attribute__new(const char *a_att_field, const char *a_att_value) {
    Sip__Type__Sdp__Attribute *p = calloc(1, sizeof(Sip__Type__Sdp__Attribute));
    if (p) {
        sip__type__sdp__attribute__init(p);
        p->a_att_field = strdup_null(a_att_field);
        p->a_att_value = strdup_null(a_att_value);
    }
    return p;
}

Sip__Type__Sdp__Attribute *sip__type__sdp__attribute__proto(const sdp_attribute_t *q) {
    if (!q)
        return NULL;
    else
        return sip__type__sdp__attribute__new(q->a_att_field, q->a_att_value);
}

Sip__Type__Sdp__Media *sip__type__sdp__media__new(const char *m_media, const char *m_port,
                                                  const char *m_number_of_port, const char *m_proto,
                                                  const char *i_info) {
    Sip__Type__Sdp__Media *p = calloc(1, sizeof(Sip__Type__Sdp__Media));
    if (p) {
        sip__type__sdp__media__init(p);
        p->m_media = strdup_null(m_media);
        p->m_port = strdup_null(m_port);
        p->m_number_of_port = strdup_null(m_number_of_port);
        p->m_proto = strdup_null(m_proto);
        p->i_info = strdup_null(i_info);
    }
    return p;
}

Sip__Type__Sdp__Media *sip__type__sdp__media__proto(const sdp_media_t *q) {
    if (!q)
        return NULL;

    Sip__Type__Sdp__Media *p = sip__type__sdp__media__new(q->m_media, q->m_port, q->m_number_of_port,
                                                          q->m_proto, q->i_info);
    if (p) {
        array__proto(&q->m_payloads, p, m_payloads);
        array__proto(&q->c_connections, p, c_connections);
        array__proto(&q->b_bandwidths, p, b_bandwidths);
        array__proto(&q->a_attributes, p, a_attributes);

        p->k_key = sip__type__sdp__key__proto(q->k_key);
    }
    return p;
}

Sip__Type__Sdp *sip__type__sdp__new(const char *v_version, const char *o_username, const char *o_sess_id,
                                    const char *o_sess_version, const char *o_nettype, const char *o_addrtype,
                                    const char *o_addr,
                                    const char *s_name, const char *i_info, const char *u_uri,
                                    const char *z_adjustments) {
    Sip__Type__Sdp *p = calloc(1, sizeof(Sip__Type__Sdp));
    if (p) {
        sip__type__sdp__init(p);
        p->v_version = strdup_null(v_version);
        p->o_username = strdup_null(o_username);
        p->o_sess_id = strdup_null(o_sess_id);
        p->o_sess_version = strdup_null(o_sess_version);
        p->o_nettype = strdup_null(o_nettype);
        p->o_addrtype = strdup_null(o_addrtype);
        p->o_addr = strdup_null(o_addr);
        p->s_name = strdup_null(s_name);
        p->i_info = strdup_null(i_info);
        p->u_uri = strdup_null(u_uri);
        p->z_adjustments = strdup_null(z_adjustments);
    }
    return p;
}

Sip__Type__Sdp *sip__type__sdp__proto(const sdp_message_t *q) {
    if (!q)
        return NULL;

    Sip__Type__Sdp *p = sip__type__sdp__new(q->v_version, q->o_username, q->o_sess_id, q->o_sess_version,
                                            q->o_nettype, q->o_addrtype, q->o_addr, q->s_name, q->i_info, q->u_uri,
                                            q->z_adjustments);
    while (p) {
        array__proto(&q->e_emails, p, e_emails);
        array__proto(&q->p_phones, p, p_phones);
        array__proto(&q->b_bandwidths, p, b_bandwidths);
        array__proto(&q->t_descrs, p, t_descrs);
        array__proto(&q->a_attributes, p, a_attributes);
        array__proto(&q->m_medias, p, m_medias);

        p->c_connection = sip__type__sdp__connection__proto(q->c_connection);
        p->k_key = sip__type__sdp__key__proto(q->k_key);

        return p;
    }
    if (p)
        sip__type__sdp__free_unpacked(p, 0);
    return NULL;
}

sdp_connection_t *sip__type__sdp__connection__unproto(const Sip__Type__Sdp__Connection *p) {
    if (!p)
        return NULL;

    sdp_connection_t *q = NULL;
    sdp_connection_init(&q);

    q->c_addr = strdup_null(p->c_addr);
    q->c_addr_multicast_int = strdup_null(p->c_addr_multicast_int);
    q->c_addr_multicast_ttl = strdup_null(p->c_addr_multicast_ttl);
    q->c_addrtype = strdup_null(p->c_addrtype);
    q->c_nettype = strdup_null(p->c_nettype);

    return q;
}

sdp_key_t *sip__type__sdp__key__unproto(const Sip__Type__Sdp__Key *p) {
    if (!p)
        return NULL;

    sdp_key_t *q = NULL;
    sdp_key_init(&q);

    q->k_keydata = strdup_null(p->k_keydata);
    q->k_keytype = strdup_null(p->k_keytype);

    return q;
}

sdp_bandwidth_t *sip__type__sdp__bandwidth__unproto(const Sip__Type__Sdp__Bandwidth *p) {
    if (!p)
        return NULL;

    sdp_bandwidth_t *q = NULL;
    sdp_bandwidth_init(&q);

    q->b_bwtype = strdup_null(p->b_bwtype);
    q->b_bandwidth = strdup_null(p->b_bandwidth);

    return q;
}

sdp_time_descr_t *sip__type__sdp__time__unproto(const Sip__Type__Sdp__Time *p) {
    if (!p)
        return NULL;

    sdp_time_descr_t *q = NULL;
    sdp_time_descr_init(&q);

    q->t_start_time = strdup_null(p->t_start_time);
    q->t_stop_time = strdup_null(p->t_stop_time);

    array__unproto(&q->r_repeats, p, r_repeats);

    return q;
}

sdp_attribute_t *sip__type__sdp__attribute__unproto(const Sip__Type__Sdp__Attribute *p) {
    if (!p)
        return NULL;

    sdp_attribute_t *q = NULL;
    sdp_attribute_init(&q);

    q->a_att_field = strdup_null(p->a_att_field);
    q->a_att_value = strdup_null(p->a_att_value);

    return q;
}

sdp_media_t *sip__type__sdp__media__unproto(const Sip__Type__Sdp__Media *p) {
    if (!p)
        return NULL;

    sdp_media_t *q = NULL;
    sdp_media_init(&q);

    q->m_media = strdup_null(p->m_media);
    q->m_port = strdup_null(p->m_port);
    q->m_number_of_port = strdup_null(p->m_number_of_port);
    q->m_proto = strdup_null(p->m_proto);
    q->i_info = strdup_null(p->i_info);

    array__unproto(&q->m_payloads, p, m_payloads);
    array__unproto(&q->c_connections, p, c_connections);
    array__unproto(&q->b_bandwidths, p, b_bandwidths);
    array__unproto(&q->a_attributes, p, a_attributes);

    q->k_key = sip__type__sdp__key__unproto(p->k_key);

    return q;
}

sdp_message_t *sip__type__sdp__unproto(const Sip__Type__Sdp *p) {
    if (!p)
        return NULL;

    sdp_message_t *m = NULL;
    sdp_message_init(&m);

    m->v_version = strdup_null(p->v_version);
    m->o_username = strdup_null(p->o_username);
    m->o_sess_id = strdup_null(p->o_sess_id);
    m->o_sess_version = strdup_null(p->o_sess_version);
    m->o_nettype = strdup_null(p->o_nettype);
    m->o_addrtype = strdup_null(p->o_addrtype);
    m->o_addr = strdup_null(p->o_addr);
    m->s_name = strdup_null(p->s_name);
    m->i_info = strdup_null(p->i_info);
    m->u_uri = strdup_null(p->u_uri);
    m->z_adjustments = strdup_null(p->z_adjustments);

    m->c_connection = sip__type__sdp__connection__unproto(p->c_connection);
    m->k_key = sip__type__sdp__key__unproto(p->k_key);

    array__unproto(&m->e_emails, p, e_emails);
    array__unproto(&m->p_phones, p, p_phones);
    array__unproto(&m->b_bandwidths, p, b_bandwidths);
    array__unproto(&m->t_descrs, p, t_descrs);
    array__unproto(&m->a_attributes, p, a_attributes);
    array__unproto(&m->m_medias, p, m_medias);

    return m;
}
