/*
 * ini.h
 *
 *  Created on: Sep 24, 2013
 *      Author: shved
 */

#ifndef INI_H_
#define INI_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef enum ini_type {

	INI_ELEMENT_TYPE_INVALID = -1,
	INI_ELEMENT_TYPE_INT,
	INI_ELEMENT_TYPE_INT64,
	INI_ELEMENT_TYPE_FLOAT,
	INI_ELEMENT_TYPE_DOUBLE,
	INI_ELEMENT_TYPE_CHAR_BUF,
	INI_ELEMENT_TYPE_CHAR_PTR,
	INI_ELEMENT_TYPE_BOOL,
	INI_ELEMENT_TYPE_INADDR,
	INI_ELEMENT_TYPE_SUBGROUP,
	INI_ELEMENT_TYPE_LAST

} ini_type_t;

typedef struct ini {

	const char * name;
	const void * value;
	enum ini_type type;
	long offset;
	long length;

} ini_t;

#define INI_BIND_ELEMENT(class, name, type, value ) \
{ \
	# name, \
	(value), \
	(type), \
	((long)&((class *)0)->name), \
	sizeof(((class *)0)->name), \
}

#define INI_BIND_END { NULL, NULL, INI_ELEMENT_TYPE_INVALID, -1, -1 }

int ini_read(void *base, const ini_t[], FILE *);
int ini_read_file(void *, const ini_t[], const char *);
void ini_free(void *, const ini_t[]);

#endif /* INI_H_ */
