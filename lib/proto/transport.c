//
// Created by shved on 29.03.2025.
//

#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include <lib/proto/sip/message.pb-c.h>

#include "transport.h"
#include "message_print.h"

#define ERROR(t, err)       t->error(err, mqtt_error_str(err), opaque)
#define RELAY(t, buf, len)  t->relay(buf, len, opaque)

/**
 *
 */
static void __invoke(ProtobufCService *service, const unsigned int index,
                     const ProtobufCMessage *input,
                     const ProtobufCClosure closure, void *opaque) {
    printf("%s %u\n", __PRETTY_FUNCTION__, index);


    // protobuf_c_message_print(input, stdout);
    //
    // const Sip__Query *query = (Sip__Query *)input;
    // Sip__Answer result;
    // sip__answer__init(&result);
    // result.response = 503;
    // result.head = query->head;
    // closure(&result.base, opaque);
}

/**
 * 
 * @param t
 * @param opaque
 */
int transport_begin(transport_t *t, void *opaque) {
    void *buf = malloc(256);
    if (!buf)
        return -1;
    const long len = mqtt_pack_connection_request(buf, 256,
                                                  t->id, NULL, NULL, 0,
                                                  t->user, t->password,
                                                  t->flags, t->alive);
    if (len < 0)
        return ERROR(t, len);
    if (!len)
        return -1;
    if (!RELAY(t, buf, len))
        return 0;
    free(buf);
    return -1;
}

/**
 * 
 * @param t
 * @param opaque
 * @return
 */
int transport_end(transport_t *t, void *opaque) {
    void *buf = malloc(16);
    if (!buf)
        return -1;
    const long len = mqtt_pack_disconnect(buf, 16);
    if (len < 0)
        return ERROR(t, len);
    if (!len)
        return -1;
    if (!RELAY(t, buf, len))
        return 0;
    free(buf);
    return -1;
}

/**
 * 
 * @param t 
 * @param opaque 
 * @return 
 */
int transport_ping(transport_t * t, void *opaque) {
    void *buf = malloc(16);
    if (!buf)
        return -1;
    const long len = mqtt_pack_ping_request(buf, 16);
    if (len < 0)
        return ERROR(t, len);
    if (!len)
        return -1;
    if (!RELAY(t, buf, len))
        return 0;
    free(buf);
    return -1;
}

/**
 *
 * @param t
 * @param opaque
 * @return
 */
int transport_subscribe(transport_t *t, void *opaque) {
    const ProtobufCServiceDescriptor *d = t->service->descriptor;
    const ProtobufCMethodDescriptor *m = d->methods;
    const char *n = d->name;
    const char *h = t->host;
    unsigned i = d->n_methods;
    while (i--) {
        char topic[256];
        long len = snprintf(topic, 256, "%s/%s/%s", h, n, m[i].name);
        if (len < 0 || len >= 256)
            return ERROR(t, MQTT_ERROR_SUBSCRIBE_FAILED);
        void *buf = malloc(512);
        if (!buf)
            return -1;
        len = mqtt_pack_subscribe_request(buf, 512, i + 1, topic, 0, NULL);
        if (len < 0)
            return ERROR(t, len);
        if (!len)
            return -1;
        if (!RELAY(t, buf, len))
            continue;
        free(buf);
        return -1;
    }
    return 0;
}

/**
 *
 * @param t
 * @param buf
 * @param size
 * @param opaque
 * @return
 */
int transport_command(transport_t *t, void *buf, const unsigned short size, void *opaque) {
    if (!buf)
        return -1;
    struct mqtt_response response;
    const long len = mqtt_unpack_response(&response, buf, size);
    if (len < 0)
        return ERROR(t, len);
    if (len == 0 || len > INT16_MAX)
        return ERROR(t, MQTT_ERROR_MALFORMED_REQUEST);
    switch (response.fixed_header.control_type) {
        case MQTT_CONTROL_CONNACK:
            switch (response.decoded.connack.return_code) {
                case MQTT_CONNACK_ACCEPTED:
                    t->subscribes = 0;
                    t->ready = 0;
                    if (transport_subscribe(t, opaque) < 0)
                        return ERROR(t, MQTT_ERROR_SUBSCRIBE_FAILED);
                    break;
                case MQTT_CONNACK_REFUSED_PROTOCOL_VERSION:
                case MQTT_CONNACK_REFUSED_IDENTIFIER_REJECTED:
                case MQTT_CONNACK_REFUSED_SERVER_UNAVAILABLE:
                case MQTT_CONNACK_REFUSED_BAD_USER_NAME_OR_PASSWORD:
                case MQTT_CONNACK_REFUSED_NOT_AUTHORIZED:
                    return ERROR(t, MQTT_ERROR_CONNECTION_REFUSED);
            }
            break;
        case MQTT_CONTROL_SUBACK:
            while (response.decoded.suback.num_return_codes--) {
                switch (response.decoded.suback.return_codes[response.decoded.suback.num_return_codes]) {
                    case MQTT_SUBACK_SUCCESS_MAX_QOS_0:
                    case MQTT_SUBACK_SUCCESS_MAX_QOS_1:
                    case MQTT_SUBACK_SUCCESS_MAX_QOS_2:
                        if (++t->subscribes == t->service->descriptor->n_methods)
                            t->ready = 1;
                        continue;
                    default:
                        return ERROR(t, MQTT_ERROR_SUBSCRIBE_FAILED);
                }
            }
            break;
        case MQTT_CONTROL_PUBLISH:
        // response.decoded.publish.packet_id;
        case MQTT_CONTROL_PUBACK:
        case MQTT_CONTROL_PUBREC:
        case MQTT_CONTROL_PUBREL:
        case MQTT_CONTROL_PUBCOMP:
        case MQTT_CONTROL_UNSUBACK:
        case MQTT_CONTROL_PINGRESP:
            break;
        default:
            return ERROR(t, MQTT_ERROR_MALFORMED_REQUEST);
    }
    return len;
}

/**
 *
 * @param service
 * @param id
 * @param user
 * @param password
 * @param host
 * @param flags MQTT_CONNECT_CLEAN_SESSION
 * @param alive
 * @param error
 * @param relay
 * @param foo
 * @return
 */
transport_t *transport_new(ProtobufCService *service,
                           const char *id,
                           const char *user,
                           const char *password,
                           const char *host,
                           const uint8_t flags,
                           const uint16_t alive,
                           int (*error)(long, const char *, void *),
                           int (*relay)(char *, unsigned short, void *), void *foo) {
    if (!service || !error || !relay)
        return NULL;

    transport_t *t = calloc(1, sizeof(transport_t));
    if (!t)
        return NULL;

    t->base.descriptor = service->descriptor;
    t->base.invoke = __invoke;
    t->service = service;
    t->error = error;
    t->relay = relay;
    t->foo1 = foo;
    t->flags = flags;
    t->alive = alive;
    t->id = id;
    t->host = host;
    t->user = user;
    t->password = password;

    return t;
}

/**
 *
 * @param t
 */
void transport_destroy(transport_t *t) {
    if (!t)
        return;
    if (t->service && t->service->destroy)
        t->service->destroy(t->service);
    free(t);
}
