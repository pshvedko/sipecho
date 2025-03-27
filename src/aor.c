/*
 * aor.c
 *
 *  Created on: Oct 24, 2013
 *      Author: shved
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>

#include "aor.h"
#include "app.h"
#include "log.h"

static struct map *__aor; /**< Address Of Record, map of @aor_t */

/**
 *
 */
static int contact_compare(osip_from_t *a, osip_to_t *b) {

	int cmp = strcasecmp(a->url->scheme, b->url->scheme);
	if (cmp == 0)
		cmp = strcasecmp(a->url->host, b->url->host);
	if (cmp == 0)
		cmp = net_port(a->url) - net_port(b->url);
	if (cmp == 0)
		if (strcmp(a->url->username ? a->url->username: "", "*") !=0 && strcmp(b->url->username ? b->url->username: "", "*")!=0)
			cmp = strcasecmp(a->url->username ? a->url->username: "", b->url->username ? b->url->username: "");

//	log_alert("%s: [%s:%s@%s:%s] = [%s:%s@%s:%s] = %i", //
//			__PRETTY_FUNCTION__,//
//			a->url->scheme, a->url->username, a->url->host, a->url->port,//
//			b->url->scheme, b->url->username, b->url->host, b->url->port, cmp);

	return cmp;
}

/**
 *
 */
static void aor_record_delete(aor_record_t *record) {

	if (record) {
		osip_contact_free(record->contact);
		osip_uri_free(record->route);
		free(record);
	}
}

/**
 *
 */
static int aor_record_cmp(aor_record_t *a, aor_record_t *b, int i) {

	int cmp = strcmp(a->protocol, b->protocol);
	if (cmp == 0)
		cmp = contact_compare(a->contact, b->contact);
	return cmp;
}

/**
 *
 */
static void aor_delete(aor_t *aor) {

	if (aor) {
		osip_from_free(aor->name);
		map_clean(&aor->contact);
		free(aor);
	}
}

/**
 *
 */
static int aor_cmp(const aor_t *a, const aor_t *b, int i) {

	return contact_compare(a->name, b->name);
}

/**
 *
 */
int aor_init() {

	__aor = map_new((map_del_t) &aor_delete, (map_cmp_t) &aor_cmp, NULL );
	if (!__aor)
		return -1;

	return 0;
}

/**
 *
 */
void aor_free() {

	map_free(__aor);
}

/**
 *
 */
static void aor_record_lookup_callback(void *foo, const char *address) {

	aor_t *key = foo;
	if (!foo)
		return;

	log_debug(FL_MAGENTA "%s: %s[%s]" FD_NORMAL,
			__PRETTY_FUNCTION__, key->preferred->contact->url->host, address);

	if (address) {
		aor_t *aor = map_find(__aor, key, 0);
		if (aor) {
			aor_record_t *record = map_find(&aor->contact, key->preferred, 0);
			if (record) {
				osip_uri_free(record->route);
				osip_uri_clone(record->contact->url, &record->route);
				free(record->route->host);
				osip_uri_set_host(record->route, strdup(address));
			}
		}
	}
	osip_contact_free(key->preferred->contact);
	osip_from_free(key->name);
	free(key->preferred);
	free(key);

}

/*
 *
 */
static void aor_record_lookup(aor_record_t *record, aor_t *aor) {

	if (!aor || !record)
		return;
	else if (dns_is_resolved(record->contact->url->host)) {
		osip_uri_clone(record->contact->url, &record->route);
		return;
	}

	log_debug(FL_MAGENTA "%s: %s" FD_NORMAL, __PRETTY_FUNCTION__, record->contact->url->host);

	aor_t *key = calloc(1, sizeof(aor_t));
	if (key) {
		key->preferred = calloc(1, sizeof(aor_record_t));
		if (key->preferred) {
			osip_from_clone(aor->name, &key->name);
			osip_contact_clone(record->contact, &key->preferred->contact);
			key->preferred->protocol = record->protocol;
			dns_lookup(record->contact->url->host, aor_record_lookup_callback, key);
			return;
		}
		free(key);
	}
}

/**
 *
 */
static aor_record_t * aor_record_new(osip_contact_t *contact, time_t expired, float q, long long id) {

	if (!contact->url || !contact->url->scheme /* || !contact->url->username */|| !contact->url->host)
		return NULL ;

	if (0 == strcmp(contact->url->username ? contact->url->username: "", "*"))
		return NULL ;

	osip_uri_param_t *expires = NULL;
	osip_uri_param_get_byname(&contact->gen_params, "expires", &expires);
	if (expired != -1)
		if (expires && expires->gvalue)
			expired = atoi(expires->gvalue);

	if (expired == 0)
		return NULL ;

	osip_uri_param_t *weight = NULL;
	osip_uri_param_get_byname(&contact->gen_params, "q", &weight);
	if (weight && weight->gvalue)
		q = atof(weight->gvalue);

	aor_record_t *record = calloc(1, sizeof(aor_record_t));
	if (record) {
		int n = osip_contact_clone(contact, &record->contact);
		if (n) {
			aor_record_delete(record);
			return NULL ;
		}
		osip_uri_param_t *transport = NULL;
		osip_uri_param_get_byname(&record->contact->url->url_params, "transport", &transport);
		if (!transport) {
			aor_record_delete(record);
			return NULL ;
		}
		record->protocol = transport->gvalue;
		record->expired = expired + time(0);
		record->id = id;
		record->q = q;
	}
	return record;
}

/**
 *
 */
static aor_t * aor_new(osip_from_t *to, osip_list_t *contacts, time_t expired, float q, long long id) {

	if (!to->url || !to->url->scheme || !to->url->username || !to->url->host)
		return NULL ;

	aor_t *aor = calloc(1, sizeof(aor_t));
	if (aor) {
		map_init(&aor->contact, (map_del_t) &aor_record_delete);
		map_add_key(&aor->contact, (map_cmp_t) &aor_record_cmp);

		if (osip_from_clone(to, &aor->name)) {
			aor_delete(aor);
			return NULL ;
		}
		int n = contacts ? osip_list_size(contacts) : 0;
		while (n--) {
			osip_contact_t *contact = osip_list_get(contacts, n);
			if (contact) {
				aor_record_t *add = aor_record_new(contact, expired, q, id);
				if (map_push(&aor->contact, add) == -1)
					aor_record_delete(add);
				else
					aor_record_lookup(add, aor);
			}
		}
		aor->expired = expired + time(0);
	}
	return aor;
}

/**
 *
 */
aor_t * aor_find(osip_to_t *name) {

	if (!name || !name->url || !name->url->scheme || !name->url->username || !name->url->host)
		return NULL ;

	aor_t key[1] = { { .name=name} };

	return map_find(__aor, key, 0);
}

/**
 * XXX: +sip.instance="<urn:uuid:bfbc8ed0-c51e-4c51-aa62-bd1c5856e4d9>" for replace contact
 *
 */
aor_t * aor_update(osip_message_t *m, time_t expired, const float q, const long long id) {

	if (!m->from || !m->from->url || !m->from->url->scheme || !m->from->url->username || !m->from->url->host)
		return NULL ;

	osip_header_t *expires = NULL;
	if (expired != -1)
		if (osip_message_get_expires(m, 0, &expires) != -1)
			if (expires)
				if (expires->hvalue)
					expired = atoi(expires->hvalue);

	aor_t key[1] = { { .name= m->from } }, *aor = map_find(__aor, key, 0);
	if (aor) {
		if (0 == strcmp(aor->name->url->username, "*"))
			return NULL ;

		int n = osip_list_size(&m->contacts);
		while (n--) {
			osip_contact_t *next = osip_list_get(&m->contacts, n);
			if (next) {
				if (next->displayname && *next->displayname == '*' && !expired) {
					map_erase(&aor->contact);
					continue;
				}
				osip_uri_param_t *transport = NULL;
				osip_uri_param_get_byname(&next->url->url_params, "transport", &transport);
				if (!transport) {
					continue;
				}
				aor_record_t del[1] = { { .protocol= transport->gvalue, .contact= next }, }, *add =
						aor_record_new(next, expired, q, id);
				if (expired != -1)
					map_pure(&aor->contact, del, 0);
				if (map_push(&aor->contact, add) == -1)
					aor_record_delete(add);
				else
					aor_record_lookup(add, aor);
			}
		}
		if (expired != -1)
			aor->expired = expired + time(0);

		return aor;
	}

	aor = aor_new(m->from, &m->contacts, expired, q, id);
	if (map_push(__aor, aor)) {
		aor_delete(aor);
		return NULL ;
	}
	return aor;
}

/**
 *
 */
aor_t * aor_push(const char *name, const char *route) {

	osip_from_t *from = NULL;
	osip_from_init(&from);
	osip_from_parse(from, name);

	osip_contact_t *contact = NULL;
	osip_contact_init(&contact);
	osip_contact_parse(contact, route);

	osip_list_t contacts;
	osip_list_init(&contacts);
	osip_list_add(&contacts, contact, 0);

	aor_t *aor = aor_new(from, &contacts, 3600 * 24 * 365, 1., 0);
	osip_list_special_free(&contacts, NULL );
	osip_contact_free(contact);
	osip_from_free(from);
	if (aor != NULL ) {
		if (map_size(&aor->contact) == 0 || map_push(__aor, aor) != 0) {
			aor_delete(aor);
			return NULL ;
		}
	}
	return aor;
}
