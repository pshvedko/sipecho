/*
 * log.c
 *
 *  Created on: Sep 24, 2013
 *      Author: shved
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <stdarg.h>
#include <ctype.h>

#include "log.h"

static const char __hex[] = "0123456789ABCDEF .";

/**
 *
 */
int hexit(const int pri, const char *bin, const unsigned l, const int n) {
    char hex[n * 4 + n / 4];

    int i = 0;
    int j = 0;

    while (i < l) {
        hex[j++] = __hex[(bin[i] >> 4)];
        hex[j++] = __hex[(bin[i] & 15)];
        hex[j++] = __hex[16];

        hex[n * 3 + n / 4 + (i % n)] = isprint(bin[i]) ? bin[i] : __hex[17];

        i++;

        if (i % 4 == 0)
            hex[j++] = __hex[16];

        if (i % n)
            continue;

    foot:

        while (j < n * 3 + n / 4)
            hex[j++] = __hex[16];

        hex[n * 3 + n / 4 + ((i - 1) % n) + 1] = 0;

        syslog(pri, "[B] %s", hex);

        j = 0;
    }
    if (j)
        goto foot;

    return 0;
}

/**
 *
 */
int logit(const int pri, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsyslog(pri, fmt, ap);
    va_end(ap);

    return 0;
}
