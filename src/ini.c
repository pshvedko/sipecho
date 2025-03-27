/*
 * ini.c
 *
 *  Created on: Sep 24, 2013
 *      Author: shved
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "ini.h"
#include "log.h"

/**
 *
 */
int ini_set_value(void *item, const int size, const ini_type_t type, const void *value, FILE *file) {
    if (!item || !value)
        return -1;

    switch (type) {
        case INI_ELEMENT_TYPE_BOOL:
            if (strcasecmp(value, "yes") != 0 && strcasecmp(value, "true") != 0)
                *((char *) item) = 0;
            else
                *((char *) item) = 1;
            break;

        case INI_ELEMENT_TYPE_CHAR_BUF:
            strncpy(item, value, size);
            break;

        case INI_ELEMENT_TYPE_CHAR_PTR:
            free(*(char **) item);
            *((char **) item) = strdup(value);
            break;

        case INI_ELEMENT_TYPE_DOUBLE:
            *((double *) item) = strtod(value, 0);
            break;

        case INI_ELEMENT_TYPE_FLOAT:
            *((float *) item) = strtof(value, 0);
            break;

        case INI_ELEMENT_TYPE_INADDR:
            inet_aton(value, item);
            break;

        case INI_ELEMENT_TYPE_INT:
            *((int *) item) = strtol(value, 0, 0);
            break;

        case INI_ELEMENT_TYPE_INT64:
            *((long long int *) item) = strtoll(value, 0, 0);
            break;

        case INI_ELEMENT_TYPE_SUBGROUP:
            if (file)
                ini_read(item, value, file);
            break;

        case INI_ELEMENT_TYPE_INVALID:
        case INI_ELEMENT_TYPE_LAST:
            break;
    }

    return 0;
}

/**
 *
 */
void ini_free(void *base, const ini_t element[]) {
    int i = 0;

    if (!base || !element)
        return;

    for (i = 0; element[i].name; i++) {
        switch (element[i].type) {
            case INI_ELEMENT_TYPE_CHAR_PTR:
                free(*(char **) ((char *) base + element[i].offset));
                break;

            case INI_ELEMENT_TYPE_SUBGROUP:
                ini_free((char *) base + element[i].offset, element[i].value);
                break;

            default:
                break;
        }
    }
}

/**
 *
 */
int ini_set_default(void *base, const ini_t element[]) {
    int i = 0;

    if (!base || !element)
        return -1;

    for (i = 0; element[i].name; i++) {
        if (element[i].type == INI_ELEMENT_TYPE_SUBGROUP)
            ini_set_default((char *) base + element[i].offset, element[i].value);
        else
            ini_set_value((char *) base + element[i].offset, element[i].length, element[i].type,
                          element[i].value, NULL);
    }
    return 0;
}

/**
 *
 */
int ini_name_cmp(const char *a, const char *b) {
    while (*a == '_')
        a++;
    return strcasecmp(a, b);
}

/**
 *
 */
int ini_read(void *base, const ini_t element[], FILE *file) {
    char buff[1024];

    while (fgets(buff, sizeof(buff), file)) {
        buff[strcspn(buff, "#;{\r\n")] = 0;
        if (*buff == 0)
            continue;

        char *name = buff;
        while (strchr(" \t", *name))
            name++;

        if (*name == '}')
            return 0;

        char *value = name + strcspn(name, " :=\t");
        value[0] = 0;
        value++;
        value = value + strspn(value, " :=\t");

        for (int i = 0; element[i].name; i++) {
            if (!ini_name_cmp(element[i].name, name)) {
                if (element[i].type == INI_ELEMENT_TYPE_SUBGROUP)
                    ini_set_value((char *) base + element[i].offset, element[i].length, element[i].type,
                                  element[i].value, file);
                else
                    ini_set_value((char *) base + element[i].offset, element[i].length, element[i].type,
                                  value, NULL);
                break;
            }
        }
    }
    return 0;
}

/**
 *
 */
int ini_read_file(void *base, const ini_t element[], const char *path) {
    FILE *file = NULL;

    if (!base || !element || !path)
        return -1;

    ini_set_default(base, element);

    if ((file = fopen(path, "r")) == NULL) {
        log_error("open: %s", strerror(errno));
        return -1;
    }

    ini_read(base, element, file);

    fclose(file);

    return 0;
}

/* end of the file env.c */
