/*
 * notify.c
 *
 *  Created on: Nov 28, 2013
 *      Author: shved
 *
 *         See: http://tools.ietf.org/html/rfc3856#section-8
 *         See: http://tools.ietf.org/html/rfc3863#section-3.1
 *         See: http://tools.ietf.org/html/rfc3863#section-4.3.1
 *         See: http://tools.ietf.org/html/rfc3339#section-5.6
 *
 *         See: http://tools.ietf.org/html/rfc5367#section-6
 *         See: http://tools.ietf.org/html/rfc4826#section-3.3
 *
 *
 *
 *  [SUBSCRIBE-MAP]                              [PRESENCE-MAP]
 *    |                                                     |
 *    +-[SUBSCRIBE]<-(1)--+             +---(2)->[PRESENCE]-+
 *    |   |               |             |               |   |
 *    |   +-[CONTACT-MAP] |             | [CONTACT-MAP]-+   |
 *    |           |       |             |            |      |
 *    |           +-[CONTACT]<----------⬝--------[*]-+      |
 *    |           |   |                 |            |      |
 *    |           |   +->[PRESENCE-MAP] |    +---[*]-+      |
 *    |           |        |            |    |              |
 *    |           |        +-[*]--->(2)-+    |   [PRESENCE]-+
 *    |           |                     |    |              |
 *    |           |  (1)<-+             |    |              |
 *    |           |       |             |    |              |
 *    |           +-[CONTACT]<----------⬝----+             ...
 *    |               |                 |
 *    |               +->[PRESENCE-MAP] |
 *    |                    |            |
 *    |                    +-[*]--->(2)-+
 *    |
 *    +-[SUBSCRIBE]
 *    |
 *   ...
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "lib/common/map.h"
#include "lib/proto/message_clone.h"

#include "notify.h"

typedef struct presence {
    Sip__Type__Address url; // key
    map_t signature; // object (signature_t)
} presence_t;

typedef struct subscribe {
    Sip__Type__Address url; // key
    map_t contact; // object (contact_t)
} subscribe_t;

typedef struct contact {
    Sip__Type__Address url; // key
    map_t presence; // refer (presence_t)
    subscribe_t *parent;
} contact_t;

typedef struct signature {
    Sip__Type__Address url;
    Sip__Type__CallId id;
    contact_t *contact; // key
} signature_t;

struct notify {
    map_t subscribe; // object (subscribe_t)
    map_t presence; // object (presence_t)
    unsigned sequence;

    int (*status)(const Sip__Type__Address *, void *);

    void *opaque;
};

static int presence_compare(const presence_t *a, const presence_t *b, int i);

static void cid_free(const Sip__Type__CallId *id) {
    if (id) {
        Sip__Type__CallId *tmp = calloc(1, sizeof(Sip__Type__CallId));
        if (tmp) {
            *tmp = *id;
            sip__type__call_id__free_unpacked(tmp, 0);
        }
    }
}

static void cid_init(Sip__Type__CallId *to, const Sip__Type__CallId *from) {
    if (!to || !from)
        return;

    protobuf_c_message_clone(&to->base, &from->base);
}

static void url_free(Sip__Type__Address *url) {
    if (url) {
        Sip__Type__Address *tmp = calloc(1, sizeof(Sip__Type__Address));
        if (tmp) {
            *tmp = *url;
            sip__type__address__free_unpacked(tmp, 0);
        }
    }
}

static void url_init(Sip__Type__Address *to, const Sip__Type__Address *from) {
    if (!to || !from)
        return;

    protobuf_c_message_clone(&to->base, &from->base);
}

static int url_port_default(const char *scheme, const char *port) {
    if (port)
        return atoi(port);
    else if (!scheme)
        return 0;
    else if (strcasecmp("SIP", scheme))
        return 5060;
    else if (strcasecmp("SIPS", scheme))
        return 5061;
    else
        return 0;
}

static int url_port(const Sip__Type__Uri *url) {
    if (url)
        return url_port_default(url->scheme, url->port);
    else
        return 0;
}

static int url_compare(const Sip__Type__Address *a, const Sip__Type__Address *b) {
    if (!a || !b || a == b || !a->url || !b->url || a->url == b->url)
        return 0;

    int cmp = strcasecmp(a->url->username ? a->url->username : "", b->url->username ? b->url->username : "");
    if (cmp == 0)
        cmp = strcasecmp(a->url->scheme ? a->url->scheme : "", b->url->scheme ? b->url->scheme : "");
    if (cmp == 0)
        cmp = strcasecmp(a->url->host ? a->url->host : "", b->url->host ? b->url->host : "");
    if (cmp == 0)
        cmp = url_port(a->url) - url_port(b->url);
    return cmp;
}

static int contact_unref(presence_t *presence, void *contact) {
    signature_t key = {.contact = contact};
    if (presence) {
        map_pure(&presence->signature, &key, 0);
    }
    return 1;
}

static void contact_free(contact_t *contact) {
    if (contact) {
        map_until(&contact->presence, (map_exe_t) &contact_unref, contact);
        map_clean(&contact->presence);
        url_free(&contact->url);
    }
    free(contact);
}

static int contact_compare(const contact_t *a, const contact_t *b, int i) {
    return url_compare(&a->url, &b->url);
}

static contact_t *contact_new(const Sip__Type__Address *url, subscribe_t *parent) {
    if (!url || !parent)
        return NULL;

    contact_t *contact = calloc(1, sizeof(contact_t));
    if (contact) {
        contact->parent = parent;
        url_init(&contact->url, url);
        map_init(&contact->presence, NULL);
        map_add_key(&contact->presence, (map_cmp_t) &presence_compare);
    }
    return contact;
}

static contact_t *contact_copy(contact_t *original, const contact_t *template, void *foo) {
    if (!original) {
        return contact_new(&template->url, template->parent);
    }
    return original;
}

static void subscribe_free(subscribe_t *subscribe) {
    if (subscribe) {
        map_clean(&subscribe->contact);
        url_free(&subscribe->url);
    }
    free(subscribe);
}

static int subscribe_compare(const subscribe_t *a, const subscribe_t *b, int i) {
    return url_compare(&a->url, &b->url);
}

static subscribe_t *subscribe_new(const Sip__Type__Address *url) {
    if (!url)
        return NULL;

    subscribe_t *subscribe = calloc(1, sizeof(subscribe_t));
    if (subscribe) {
        url_init(&subscribe->url, url);
        map_init(&subscribe->contact, (map_del_t) &contact_free);
        map_add_key(&subscribe->contact, (map_cmp_t) &contact_compare);
    }
    return subscribe;
}

static subscribe_t *subscribe_copy(subscribe_t *original, const subscribe_t *template, void *foo) {
    if (!original) {
        return subscribe_new(&template->url);
    }
    return original;
}

static void signature_free(signature_t *signature) {
    if (signature) {
        url_free(&signature->url);
        cid_free(&signature->id);
    }
    free(signature);
}

static int signature_compare(const signature_t *a, const signature_t *b, int i) {
    return url_compare(&a->contact->url, &b->contact->url);
}

static signature_t *signature_new(const Sip__Type__Address *url, const Sip__Type__CallId *id,
                                  contact_t *contact) {
    if (!url || !id || !contact)
        return NULL;

    signature_t *signature = calloc(1, sizeof(signature_t));
    if (signature) {
        url_init(&signature->url, url);
        cid_init(&signature->id, id);
        signature->contact = contact;
    }
    return signature;
}

static signature_t *signature_copy(signature_t *original, const signature_t *template, void *foo) {
    if (!original) {
        return signature_new(&template->url, &template->id, template->contact);
    }
    url_free(&original->url);
    url_init(&original->url, &template->url);
    cid_free(&original->id);
    cid_init(&original->id, &template->id);

    return original;
}

static int presence_unref(signature_t *signature, void *presence) {
    if (signature && signature->contact) {
        map_pure(&signature->contact->presence, presence, 0);
    }
    return 1;
}

static void presence_free(presence_t *presence) {
    if (presence) {
        map_until(&presence->signature, (map_exe_t) &presence_unref, presence);
        map_clean(&presence->signature);
        url_free(&presence->url);
    }
    free(presence);
}

static int presence_compare(const presence_t *a, const presence_t *b, int i) {
    return url_compare(&a->url, &b->url);
}

static presence_t *presence_new(const Sip__Type__Address *url) {
    if (!url)
        return NULL;

    presence_t *presence = calloc(1, sizeof(presence_t));
    if (presence) {
        url_init(&presence->url, url);
        map_init(&presence->signature, (map_del_t) &signature_free);
        map_add_key(&presence->signature, (map_cmp_t) &signature_compare);
    }
    return presence;
}

static presence_t *presence_copy(presence_t *original, const presence_t *template, void *foo) {
    if (!original) {
        return presence_new(&template->url);
    }
    return original;
}

int notify_delete(Sip__Notify *notify, const Sip__Type__Address *from, const Sip__Type__Address *contact_0,
                  const Sip__Type__Address *to) {
    if (!notify)
        return -1;
    if (!from)
        return -1;

    if (contact_0) {
        subscribe_t _subscribe = {.url = *from};
        subscribe_t *subscribe = map_find(&notify->subscribe, &_subscribe, 0);
        if (!subscribe)
            return 0;

        if (to) {
            contact_t _contact = {.url = *contact_0, .parent = subscribe};
            contact_t *contact = map_find(&subscribe->contact, &_contact, 0);
            if (!contact)
                return 0;

            presence_t _presence = {.url = *to};
            presence_t *presence = map_get(&contact->presence, &_presence, 0);
            if (presence) {
                signature_t _signature = {.contact = contact};
                signature_t *signature = map_get(&presence->signature, &_signature, 0);
                if (signature) {
                    signature_free(signature);
                    if (map_size(&presence->signature) == 0)
                        map_pure(&notify->presence, presence, 0);
                }
                if (map_size(&contact->presence))
                    return 0;
            } else
                return 0;
        }
        contact_t _contact = {.url = *contact_0, .parent = subscribe};
        contact_t *contact = map_get(&subscribe->contact, &_contact, 0);
        if (contact) {
            contact_free(contact);
            if (map_size(&subscribe->contact))
                return 0;
        } else
            return 0;
    }
    subscribe_t _subscribe = {.url = *from};
    subscribe_t *subscribe = map_get(&notify->subscribe, &_subscribe, 0);
    if (subscribe)
        subscribe_free(subscribe);

    return 0;
}

int notify_update(Sip__Notify *notify, const Sip__Type__Address *from, const Sip__Type__Address *contact_0,
                  const Sip__Type__Address *to, const Sip__Type__CallId *id, int online,
                  void (*submit)(const Sip__Query *, void *), void *opaque) {
    map_flag_t flag = map_flag_update;
    if (!notify)
        return -1;
    if (!from)
        return -1;
    if (!contact_0)
        return -1;
    if (!to)
        return -1;
    if (!id)
        return -1;

    subscribe_t _subscribe = {.url = *from};
    subscribe_t *subscribe = map_push_find(&notify->subscribe, &_subscribe, (map_cpy_t) &subscribe_copy, NULL, &flag);
    if (!subscribe)
        return -1;

    contact_t _contact = {.url = *contact_0, .parent = subscribe};
    contact_t *contact = map_push_find(&subscribe->contact, &_contact, (map_cpy_t) &contact_copy, NULL, &flag);
    if (!contact)
        return -1;

    presence_t _presence = {.url = *to};
    presence_t *presence = map_push_find(&notify->presence, &_presence, (map_cpy_t) &presence_copy, NULL, &flag);
    if (!presence)
        return -1;

    signature_t _signature = {.url = *from, .id = *id, .contact = contact};
    signature_t *signature = map_push_find(&presence->signature, &_signature, (map_cpy_t) &signature_copy, NULL, &flag);
    if (!signature)
        return -1;

    map_push_back(&contact->presence, presence);

    char branch[64];
    char number[16];

    snprintf(branch, sizeof(branch), "%s-%zx-%p%08lx", "z9hG4bK", time(0), presence, random());
    snprintf(number, sizeof(number), "%i", ++notify->sequence);

    Sip__Type__Pair pair[] = {SIP__TYPE__PAIR__INIT, SIP__TYPE__PAIR__INIT, SIP__TYPE__PAIR__INIT};
    Sip__Type__Pair *others[] = {pair + 0, pair + 1}, *via_params[] = {pair + 2};
    Sip__Type__ContentType content_type[] = {SIP__TYPE__CONTENT_TYPE__INIT};
    Sip__Type__Cseq cseq[] = {SIP__TYPE__CSEQ__INIT};
    Sip__Type__Via via[] = {SIP__TYPE__VIA__INIT}, *vias[] = {via};
    Sip__Query note[] = {SIP__QUERY__INIT};
    Sip__Head head[] = {SIP__HEAD__INIT};

    note->request = contact->url.url;
    note->head = head;
    note->head->version = "SIP/2.0";
    note->head->via = vias;
    note->head->via[0]->host = NOTIFY_DEFAULT_VIA_HOSTNAME;
    note->head->via[0]->version = NOTIFY_DEFAULT_VIA_VERSION;
    note->head->via[0]->protocol = NOTIFY_DEFAULT_VIA_PROTOCOL;
    note->head->via[0]->via_params = via_params;
    note->head->via[0]->via_params[0]->name = "branch";
    note->head->via[0]->via_params[0]->value = branch;
    note->head->via[0]->n_via_params = 1;
    note->head->n_via = 1;
    note->head->to = &signature->url;
    note->head->from = &presence->url;
    note->head->call_id = &signature->id;
    note->head->cseq = cseq;
    note->head->cseq->method = "NOTIFY";
    note->head->cseq->number = number;
    note->head->other = others;
    note->head->other[0]->name = "Event";
    note->head->other[0]->value = "presence";
    note->head->other[1]->name = "Subscription-State";
    note->head->other[1]->value = "active";
    note->head->n_other = 2;
    note->head->content_type = content_type;
    note->head->content_type->type = "application";
    note->head->content_type->subtype = "pidf+xml";

    char __entity[512];
    snprintf(__entity, sizeof(__entity), "%s:%s@%s", presence->url.url->scheme, presence->url.url->username,
             presence->url.url->host);

    Sip__Type__Pidf__Presence__Tuple__Status status[] = {SIP__TYPE__PIDF__PRESENCE__TUPLE__STATUS__INIT};
    Sip__Type__Pidf__Presence__Tuple tuple[] = {SIP__TYPE__PIDF__PRESENCE__TUPLE__INIT};
    Sip__Type__Pidf__Presence__Tuple *tuples[] = {tuple};
    Sip__Type__Pidf__Presence __presence[] = {SIP__TYPE__PIDF__PRESENCE__INIT};
    Sip__Type__Pidf pidf[] = {SIP__TYPE__PIDF__INIT};

    note->pidf = pidf;
    note->pidf->presence = __presence;
    note->pidf->presence->__entity = __entity;
    note->pidf->presence->tuple = tuples;
    note->pidf->presence->tuple[0]->__id = "1";
    note->pidf->presence->tuple[0]->contact = __entity;
    note->pidf->presence->tuple[0]->status = status;
    note->pidf->presence->tuple[0]->status->basic =
    ((online == 1)
     || (online == -1 && notify->status && notify->status(&presence->url, notify->opaque)))
        ? "open"
        : "closed";
    note->pidf->presence->tuple[0]->timestamp = 0;
    note->pidf->presence->n_tuple = 1;

    if (submit)
        submit(note, opaque);

    return 0;
}

int notify_change(Sip__Notify *notify, const Sip__Type__Address *to, int online,
                  void (*submit)(const Sip__Query *, void *), void *opaque) {
    if (!notify)
        return -1;
    if (!to)
        return -1;

    presence_t _presence = {.url = *to};
    presence_t *presence = map_find(&notify->presence, &_presence, 0);
    if (!presence)
        return 0;

    map_iter_t *iter = map_iter_begin(&presence->signature);
    while (*iter) {
        signature_t *signature = map_iter_down(&presence->signature, &iter);
        if (signature) {
            char branch[64];
            char number[16];

            snprintf(branch, sizeof(branch), "%s-%zx-%p%08lx", "z9hG4bK", time(0), presence, random());
            snprintf(number, sizeof(number), "%i", ++notify->sequence);

            Sip__Type__Pair pair[] = {SIP__TYPE__PAIR__INIT, SIP__TYPE__PAIR__INIT, SIP__TYPE__PAIR__INIT};
            Sip__Type__Pair *others[] = {pair + 0, pair + 1}, *via_params[] = {pair + 2};
            Sip__Type__ContentType content_type[] = {SIP__TYPE__CONTENT_TYPE__INIT};
            Sip__Type__Cseq cseq[] = {SIP__TYPE__CSEQ__INIT};
            Sip__Type__Via via[] = {SIP__TYPE__VIA__INIT}, *vias[] = {via};
            Sip__Query note[] = {SIP__QUERY__INIT};
            Sip__Head head[] = {SIP__HEAD__INIT};

            note->request = signature->contact->url.url;
            note->head = head;
            note->head->version = "SIP/2.0";
            note->head->via = vias;
            note->head->via[0]->host = NOTIFY_DEFAULT_VIA_HOSTNAME;
            note->head->via[0]->version = NOTIFY_DEFAULT_VIA_VERSION;
            note->head->via[0]->protocol = NOTIFY_DEFAULT_VIA_PROTOCOL;
            note->head->via[0]->via_params = via_params;
            note->head->via[0]->via_params[0]->name = "branch";
            note->head->via[0]->via_params[0]->value = branch;
            note->head->via[0]->n_via_params = 1;
            note->head->n_via = 1;
            note->head->to = &signature->url;
            note->head->from = &presence->url;
            note->head->call_id = &signature->id;
            note->head->cseq = cseq;
            note->head->cseq->method = "NOTIFY";
            note->head->cseq->number = number;
            note->head->other = others;
            note->head->other[0]->name = "Event";
            note->head->other[0]->value = "presence";
            note->head->other[1]->name = "Subscription-State";
            note->head->other[1]->value = "active";
            note->head->n_other = 2;
            note->head->content_type = content_type;
            note->head->content_type->type = "application";
            note->head->content_type->subtype = "pidf+xml";

            char __entity[512];
            snprintf(__entity, sizeof(__entity), "%s:%s@%s", presence->url.url->scheme,
                     presence->url.url->username, presence->url.url->host);

            Sip__Type__Pidf__Presence__Tuple__Status status[] = {
                SIP__TYPE__PIDF__PRESENCE__TUPLE__STATUS__INIT
            };
            Sip__Type__Pidf__Presence__Tuple tuple[] = {SIP__TYPE__PIDF__PRESENCE__TUPLE__INIT};
            Sip__Type__Pidf__Presence__Tuple *tuples[] = {tuple};
            Sip__Type__Pidf__Presence __presence[] = {SIP__TYPE__PIDF__PRESENCE__INIT};
            Sip__Type__Pidf pidf[] = {SIP__TYPE__PIDF__INIT};

            note->pidf = pidf;
            note->pidf->presence = __presence;
            note->pidf->presence->__entity = __entity;
            note->pidf->presence->tuple = tuples;
            note->pidf->presence->tuple[0]->__id = "1";
            note->pidf->presence->tuple[0]->contact = __entity;
            note->pidf->presence->tuple[0]->status = status;
            note->pidf->presence->tuple[0]->status->basic =
            ((online == 1)
             || (online == -1 && notify->status
                 && notify->status(&presence->url, notify->opaque)))
                ? "open"
                : "closed";
            note->pidf->presence->tuple[0]->timestamp = 0;
            note->pidf->presence->n_tuple = 1;

            if (submit)
                submit(note, opaque);
        }
    }

    return 0;
}

void notify_free(Sip__Notify *notify) {
    if (notify) {
        map_clean(&notify->subscribe);
        map_clean(&notify->presence);
    }
    free(notify);
}

Sip__Notify *notify_new(int (*status)(const Sip__Type__Address *, void *), void *opaque) {
    Sip__Notify *notify = calloc(1, sizeof(Sip__Notify));
    if (notify) {
        map_init(&notify->subscribe, (map_del_t) &subscribe_free);
        map_add_key(&notify->subscribe, (map_cmp_t) &subscribe_compare);
        map_init(&notify->presence, (map_del_t) &presence_free);
        map_add_key(&notify->presence, (map_cmp_t) &presence_compare);
        notify->opaque = opaque;
        notify->status = status;
    }
    return notify;
}
