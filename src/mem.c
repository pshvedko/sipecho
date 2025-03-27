/*
 * mem.c
 *
 *  Created on: Sep 27, 2013
 *      Author: shved
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "mem.h"
#include "log.h"

/**
 *
 */
mem_t *mem_new(void *buffer, const unsigned short length, struct sockaddr *a, const unsigned l) {
    mem_t *chunk = length ? calloc(1, sizeof(mem_t)) : NULL;
    if (!chunk)
        return NULL;

    chunk->peer->l = l ? l : sizeof(chunk->peer->a);
    if (a)
        memcpy(&chunk->peer->a, a, l);

    if (!buffer)
        chunk->buffer = malloc(length);
    else {
        chunk->buffer = buffer;
        chunk->end = length;
    }
    chunk->length = length;

    if (chunk->buffer)
        return chunk;

    free(chunk);

    return NULL;
}

/**
 *
 * @return
 */
mem_t *mem_new_printf(const char *restrict fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    char *buf;
    const int n = vasprintf(&buf, fmt, ap);
    va_end(ap);
    return mem_new(buf, n, 0, 0);
}

/**
 *
 */
void mem_align(mem_t *chunk) {
    if (!chunk || !chunk->begin)
        return;

    log_debug("%s: %i", __PRETTY_FUNCTION__, chunk->end - chunk->begin);

    memcpy(chunk->buffer, chunk->buffer + chunk->begin, chunk->end - chunk->begin);

    chunk->end -= chunk->begin;
    chunk->done -= chunk->begin;
    chunk->begin -= chunk->begin;
}

/**
 *
 */
void mem_reset(mem_t *chunk) {
    if (chunk) {
        chunk->begin = chunk->done = chunk->end = 0;
    }
}

/**
 *
 */
void mem_free(mem_t *chunk) {
    if (chunk) {
        free(chunk->buffer);
        free(chunk);
    }
}

/**
 *
 */
void mem_delete(void *chunk) {
    mem_free(chunk);
}

/**
 *
 * @param mem
 * @return
 */
int mem_empty(mem_t *mem) {
    if (!mem)
        return 0;
    return mem->end == mem->begin;
}
