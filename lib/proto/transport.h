//
// Created by shved on 29.03.2025.
//

#ifndef TRANSPORT_H
#define TRANSPORT_H

#include <stdlib.h>

#include <protobuf-c/protobuf-c.h>

#include "lib/mqtt/mqtt.h"

typedef struct transport {
    ProtobufCService base, *service;

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
    int subscribes;

    long ready;
    void *foo1;
} transport_t;

void transport_destroy(transport_t *);

transport_t *transport_new(ProtobufCService *,
                           const char *id,
                           const char *user,
                           const char *password,
                           const char *host,
                           uint8_t flags,
                           uint16_t alive,
                           int (*)(long, const char *, void *),
                           int (*)(char *, unsigned short, void *), void *);

int transport_begin(transport_t *, void *);

int transport_command(transport_t *, void *, unsigned short, void *);

int transport_end(transport_t *, void *);

int transport_ping(transport_t *, void *);

int transport_subscribe(transport_t *, void *);

#endif //TRANSPORT_H
