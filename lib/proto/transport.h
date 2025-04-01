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

    int (*error)(void *, long, const char *);

    int (*relay)(void *, char *, unsigned short);

    const char *to;
    const char *user;
    const char *password;
    const char *host;

    uint8_t flags;
    uint16_t alive;
    uint16_t id;

    int (*begin)(struct transport *, unsigned short, void *);

    int (*subscribe)(struct transport *, void *);

    int (*ping)(struct transport *, void *);

    int (*command)(struct transport *, void *, unsigned short, void *);

    int (*destroy)(struct transport *, int);

    int (*end)(struct transport *, void *);

    unsigned short port;
    int subscribes;
    long ready;
    void *foo;

    void (*methods[])();

} transport_t;

transport_t *transport_new(ProtobufCService *,
                           const char *to,
                           const char *user,
                           const char *password,
                           const char *host,
                           uint8_t flags,
                           uint16_t alive,
                           int (*)(void *, long, const char *),
                           int (*)(void *, char *, unsigned short), void *);

#endif //TRANSPORT_H
