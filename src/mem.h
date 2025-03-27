/*
 * mem.h
 *
 *  Created on: Sep 27, 2013
 *      Author: shved
 */

#ifndef MEM_H_
#define MEM_H_

#include <sys/types.h>
#include <sys/socket.h>

typedef struct mem {
    char *buffer;
    unsigned short length;
    unsigned short begin;
    unsigned short done;
    unsigned short end;

    struct {
        struct sockaddr a;
        socklen_t l;
    } peer[1];
} mem_t;

#define MEM_INITIALIZER	{ 0, 0, 0, 0, 0, { { 0 }, 0 } }

mem_t *mem_new(void *, unsigned short, struct sockaddr *, unsigned);

mem_t *mem_new_printf(const char *restrict, ...);

int mem_empty(mem_t *);

void mem_align(mem_t *);

void mem_reset(mem_t *);

void mem_free(mem_t *);

void mem_delete(void *);

#endif /* MEM_H_ */
