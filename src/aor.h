/*
 * aor.h
 *
 *  Created on: Oct 24, 2013
 *      Author: shved
 */

#ifndef AOR_H_
#define AOR_H_

#include <sys/time.h>

#include <stdlib.h>

#include <osip2/osip.h>
#include <osipparser2/osip_message.h>

#include "lib/common/map.h"

#include "net.h"

/**
 * Definition of the AOR Contact header.
 * @struct aor_record
 */
typedef struct aor_record {
    const char *protocol;
    long long id; /**< net event id */
    unsigned flag; /**< flag to use back the TCP connection */
    float q;
    osip_contact_t *contact; /**< sip contact */
    osip_uri_t *route; /**< route for contact URL resolved via DNS */
    time_t expired; /**< registration expire time */
} aor_record_t;

/**
 * Definition of the AOR header.
 * @struct aor
 */
typedef struct aor {
    osip_from_t *name; /**< registered name */
    time_t expired; /**< registration expire time */
    map_t contact; /**< map of registered contacts @aor_record_t */
    aor_record_t *preferred; /**< link to preferred contact */
} aor_t;

int aor_init();

void aor_free();

void aor_walk(int (*exe)(aor_t *, void *), void *foo);

aor_t *aor_update(osip_message_t *, time_t, float, long long);

aor_t *aor_find(osip_to_t *);

aor_t *aor_push(const char *, const char *);

#endif /* AOR_H_ */
