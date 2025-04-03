//
// Created by shved on 29.03.2025.
//

#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include "message_print.h"

#include "transport.h"

#define ERROR(t, err)       t->error(t->foo, err, mqtt_error_str(err))
#define RELAY(t, buf, len)  t->relay(t->foo, buf, len)

/**
 *
 * @param t
 * @return
 */
static uint16_t packet_id(transport_t *t) {
    return ++t->id;
}

/**
 *
 * @code
 * sip: [I] > SIP/2.0 503 transport Unavailable
 * sip: [I]   Via: SIP/2.0/UDP 192.168.0.66:11926;rport=11926;branch=z9hG4bK-73p695833916343754516r
 * sip: [I]   Record-Route: <sip:sip.home.lan:5060;transport=UDP;lr>
 * sip: [I]   From: <sip:1000@192.168.0.100>;tag=72g9131061675272755852m
 * sip: [I]   To: <sip:1000@192.168.0.100>
 * sip: [I]   Call-ID: 6b8c5f60-4e36-40ab-ad2d-d778ef9b778f
 * sip: [I]   CSeq: 12557 REGISTER
 * sip: [I]   Date: Mon, 31 Mar 2025 12:25:38 GMT
 * sip: [I]   Content-Length: 0
 * sip: [I]
 *
 * closure(NULL, opaque);
 * @endcode
 *
 * @param service
 * @param index
 * @param input
 * @param closure
 * @param opaque
 */
static void invoke(ProtobufCService *service, const unsigned int index,
                   const ProtobufCMessage *input,
                   const ProtobufCClosure closure, void *opaque) {
    struct transport *t = (transport_t *) service;
    unsigned char message[4096];
    const size_t size = protobuf_c_message_pack(input, message);
    if (size > 0) {
        void *buf = malloc(256 + size);
        if (buf) {
            const ProtobufCServiceDescriptor *d = t->base->descriptor;
            const ProtobufCMethodDescriptor *m = d->methods;
            char topic[128];
            sprintf(topic, "%s/%s/%s/%hu", t->to, m[index].name, t->host, t->port);
            const ssize_t len = mqtt_pack_publish_request(buf, 256 + size, topic,
                                                          packet_id(t), message, size, 0);
            if (len > 0) {
                if (!RELAY(t, buf, len))
                    return;
            } else if (len < 0)
                ERROR(t, len);
            free(buf);
        }
    }
    if (closure)
        closure(NULL, opaque);
}

struct topic {
    const char *name;
    const char *method;
    const char *host;
    const char *port;
    unsigned short size;
};

/**
 *
 * @param name
 * @param size
 * @return
 */
static struct topic get_topic(const char *name, uint16_t size) {
    struct topic topic = {};

    char *method = memchr(name, '/', size);
    if (!method)
        return topic;
    *method = '\0';
    method++;
    size -= method - name;

    char *host = memchr(method, '/', size);
    if (!host)
        return topic;
    *host = '\0';
    host++;
    size -= host - method;

    char *port = memchr(host, '/', size);
    if (!port)
        return topic;
    *port = '\0';
    port++;
    size -= port - host;

    topic.name = name;
    topic.method = method;
    topic.host = host;
    topic.port = port;
    topic.size = size;

    return topic;
}

/**
 * 
 * @param t
 * @param name 
 * @param size 
 * @return 
 */
static unsigned get_method(transport_t *t, const void *name, const uint16_t size) {
    const struct topic topic = get_topic(name, size);
    if (topic.method != NULL) {
        unsigned i = t->base->descriptor->n_methods;
        while (i--) {
            if (strcmp(topic.method, t->base->descriptor->methods[i].name) == 0) {
                return i + 1;
            }
        }
    }
    return 0;
}

/**
 *
 */
struct token {
    struct transport *transport;
    unsigned index;
};

/**
 * 
 * @param transport 
 * @param index 
 * @return 
 */
static struct token *token_new(struct transport *transport, const unsigned index) {
    struct token *t = malloc(sizeof(struct token));
    if (t) {
        t->transport = transport;
        t->index = index;
    }
    return t;
}

/**
 * 
 * @param message 
 * @param opaque 
 */
static void callback(const ProtobufCMessage *message, void *opaque) {
    if (message) {
        const struct token *token = opaque;
        if (token)
            invoke(token->transport->base, token->index, message, NULL, NULL);
    }
    free(opaque);
}

/**
 *
 * @param t
 * @param input
 * @param opaque
 */
static void input(transport_t *t, const struct mqtt_response_publish input, void *opaque) {
    unsigned i = get_method(t, input.topic_name, input.topic_name_size);
    if (!i)
        return;
    ProtobufCMessage *message = protobuf_c_message_unpack(t->base->descriptor->methods[--i].output, NULL,
                                                          input.application_message_size,
                                                          input.application_message);
    if (message)
        t->methods[i](t->base, message, callback, token_new(t, i));
    protobuf_c_message_free_unpacked(message, NULL);
}

/**
 * 
 * @param t
 * @param port
 * @param opaque
 */
static int begin(transport_t *t, const unsigned short port, void *opaque) {
    void *buf = malloc(256);
    if (!buf)
        return -1;

    t->port = port;

    char id[strlen(t->base->descriptor->name) + strlen(t->host) + 8];
    sprintf(id, "%hu", port);
    strcat(id, "@");
    strcat(id, t->host);
    strcat(id, "@");
    strcat(id, t->base->descriptor->name);

    const long len = mqtt_pack_connection_request(buf, 256,
                                                  id, NULL, NULL, 0,
                                                  t->user, t->password,
                                                  t->flags, t->alive);
    if (len > 0 && !RELAY(t, buf, len))
        return 0;
    else
        free(buf);
    if (len < 0)
        return ERROR(t, len);
    return -1;
}

/**
 *
 * @param t
 * @param err
 */
static int destroy(transport_t *t, const int err) {
    if (t) {
        if (t->base->destroy)
            t->base->destroy(t->base);
        free(t);
    }
    return err;
}

/**
 * 
 * @param t
 * @param opaque
 * @return
 */
static int end(transport_t *t, void *opaque) {
    void *buf = malloc(16);
    if (!buf)
        return -1;
    const long len = mqtt_pack_disconnect(buf, 16);
    if (len > 0 && !RELAY(t, buf, len))
        return 0;
    else
        free(buf);
    if (len < 0)
        return ERROR(t, len);
    return -1;
}

/**
 * 
 * @param t 
 * @param opaque 
 * @return 
 */
static int ping(transport_t *t, void *opaque) {
    void *buf = malloc(16);
    if (!buf)
        return -1;
    const long len = mqtt_pack_ping_request(buf, 16);
    if (len > 0 && !RELAY(t, buf, len))
        return 0;
    else
        free(buf);
    if (len < 0)
        return ERROR(t, len);
    return -1;
}

/**
 *
 * @param t
 * @param opaque
 * @return
 */
static int subscribe(transport_t *t, void *opaque) {
    const ProtobufCServiceDescriptor *d = t->base->descriptor;
    const ProtobufCMethodDescriptor *m = d->methods;
    unsigned i = d->n_methods;
    while (i--) {
        char topic[256];
        long len = snprintf(topic, 256, "%s/%s/%s/%hu", d->name, m[i].name, t->host, t->port);
        if (len < 0 || len >= 256)
            return ERROR(t, MQTT_ERROR_SUBSCRIBE_FAILED);
        void *buf = malloc(512);
        if (!buf)
            return -1;
        len = mqtt_pack_subscribe_request(buf, 512, i + 1, topic, 0, NULL);
        if (len > 0 && !RELAY(t, buf, len))
            continue;
        else
            free(buf);
        if (len < 0)
            return ERROR(t, len);
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
static int command(transport_t *t, void *buf, const unsigned short size, void *opaque) {
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
                    if (subscribe(t, opaque) < 0)
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
                        if (++t->subscribes == t->base->descriptor->n_methods)
                            t->ready = 1;
                        continue;
                    default:
                        return ERROR(t, MQTT_ERROR_SUBSCRIBE_FAILED);
                }
            }
            break;
        case MQTT_CONTROL_PUBLISH:
            input(t, response.decoded.publish, opaque);
            break;
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

    return (int) len;
}

/**
 *
 * @param service
 * @param to
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
                           const char *to,
                           const char *user,
                           const char *password,
                           const char *host,
                           const uint8_t flags,
                           const uint16_t alive,
                           int (*error)(void *, long, const char *),
                           int (*relay)(void *, char *, unsigned short), void *foo) {
    if (!service || !error || !relay)
        return NULL;

    transport_t *t = calloc(1, sizeof(transport_t) + service->descriptor->n_methods * sizeof(void(*)()));
    if (!t)
        return NULL;

    t->error = error;
    t->relay = relay;
    t->flags = flags;
    t->alive = alive;
    t->foo = foo;
    t->to = to;
    t->host = host;
    t->user = user;
    t->password = password;

    t->begin = begin;
    t->ping = ping;
    t->command = command;
    t->subscribe = subscribe;
    t->end = end;
    t->destroy = destroy;

    memcpy(t->base, service, sizeof(ProtobufCService));
    memcpy(t->methods, service + 1, service->descriptor->n_methods * sizeof(void(*)()));

    t->base->invoke = invoke;

    return t;
}
