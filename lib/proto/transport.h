//
// Created by shved on 29.03.2025.
//

#ifndef TRANSPORT_H
#define TRANSPORT_H

#include <stdlib.h>

#include <protobuf-c/protobuf-c.h>

#include "lib/mqtt/mqtt.h"

typedef struct transport {
    ProtobufCService base[1];

    void (*invoke)(ProtobufCService *, unsigned, const ProtobufCMessage *, void (*)(const ProtobufCMessage *, void *),
                   void *);

    int (*error)(long, const char *, void *);

    int (*relay)(char *, unsigned short, void *);

    struct {
        const char *host;
        unsigned short port;
    } peer[1];

    const char *id;
    const char *user;
    const char *password;
    const char *host;

    uint8_t flags;
    uint16_t alive;

    int (*begin)(struct transport *, unsigned short, void *);

    int (*subscribe)(struct transport *, void *);

    int (*ping)(struct transport *, void *);

    int (*command)(struct transport *, void *, unsigned short, void *);

    void (*destroy)(struct transport *);

    int (*end)(struct transport *, void *);

    unsigned short port;
    int subscribes;
    long ready;
    void *foo;

    void (*methods[])();
} transport_t;

transport_t *transport_new(ProtobufCService *,
                           const char *id,
                           const char *user,
                           const char *password,
                           const char *host,
                           uint8_t flags,
                           uint16_t alive,
                           int (*)(long, const char *, void *),
                           int (*)(char *, unsigned short, void *), void *);

#endif //TRANSPORT_H
