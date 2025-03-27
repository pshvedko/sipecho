/*
 * xml.h
 *
 *  Created on: Sep 24, 2013
 *      Author: shved
 */

#ifndef XML_H_
#define XML_H_

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define xml_max_DEEP	512

typedef enum xml_type {

	xml_attribute_node,
	xml_document_node,
	xml_element_node,
	xml_empty_node,
	xml_text_node,

} xml_type_t;

typedef enum xml_brace {

	xml_brace_mandatory,
	xml_brace_similar,
	xml_brace_none,
	xml_brace_any,

} xml_brace_t;

typedef enum xml_block {

	xml_block_follow,
	xml_block_comment,

} xml_block_t;

typedef enum xml_shape {

	xml_shape_open,
	xml_shape_single,
	xml_shape_close,

} xml_shape_t;

typedef struct xml_text {

	const char *value;
	unsigned length;
	int needfree;

} xml_text_t;

#define xml_text_INIT( x ) \
{ \
	.value = x + 0, \
	.length = sizeof( "" x "" ) - 1, \
	.needfree = 0, \
}

#define xml_text_INITIALIZER xml_text_INIT()

typedef struct xml_tag {

	struct xml_text name;
	enum xml_brace brace;
	enum xml_block block;

} xml_tag_t;

#define xml_tag_INITIALIZER( n, m, b ) \
{ \
	.name = { .value = #n, .length = sizeof( #n ) - 1 }, \
	.brace = xml_brace_ ## m, \
	.block = xml_block_ ## b, \
}

typedef struct xml_element {

	int id;
	int level;
	enum xml_shape shape;
	enum xml_type type;
	struct xml_text name;
	struct xml_text content;
	struct xml_element *attribute;
	struct xml_element *children;
	struct xml_element *parent;
	struct xml_element *next;
	struct xml_element *last;
	const struct xml_tag *tag;

} xml_element_t;

#define xml_element_INITIALIZER \
{ \
	.id = 0, \
	.level = 0, \
	.shape = xml_shape_open, \
	.type = xml_empty_node, \
	.name = xml_text_INITIALIZER, \
	.content = xml_text_INITIALIZER, \
	.attribute = NULL, \
	.children = NULL, \
	.parent = NULL, \
	.next = NULL, \
	.last = NULL, \
	.tag = NULL, \
}

typedef struct xml {

	struct xml_element document;

} xml_t;

#define xml_INITIALIZER \
{ \
	.document = xml_element_INITIALIZER, \
}

typedef struct xml_entity {

	struct xml_text entity;
	struct xml_text symbol;
	int number;

} xml_entity_t;

#define xml_entity_INITIALIZER( s, n, e ) \
{ \
	.entity = { value: #e, length: sizeof( #e ) - 1 }, \
	.symbol = { value: ( s ), length: sizeof( s ) - 1 }, \
	.number = n, \
}

#define xml_TEXT(__text)	xml_text_set(#__text, sizeof(#__text) - 1)

int xml_is_comment(const char *, int);
int xml_is_closing(const char *, struct xml_element *);
int xml_text_is_empty(struct xml_text *);
int xml_text_cpy(char *, int, const struct xml_text);
int xml_text_cmp(const struct xml_text *, const struct xml_text *);
int xml_text_cmp3(const struct xml_text *, const char *);
int xml_text_cmp4(const struct xml_text *, const char *, unsigned);
int xml_text_merge(char *, int, int, const struct xml_text *);

int xml_feed(struct xml *, const char *, size_t);
int xml_seed(struct xml *, char [], size_t, char);
int xml_node_walk(struct xml_element *, int (*)(struct xml_element *, int));
int xml_text_cmp2(const struct xml_text, const struct xml_text);
int xml_text_is_null(const struct xml_text *);

unsigned xml_text_length(const struct xml_text *);
const char * xml_text_value(const struct xml_text *);
int xml_text_int32(const struct xml_text *);
long long int xml_text_int64(const struct xml_text *);
unsigned int xml_text_uint32(const struct xml_text *);
unsigned long long int xml_text_uint64(const struct xml_text *);
float xml_text_float(const struct xml_text *);
double xml_text_double(const struct xml_text *);
int xml_text_bool(const struct xml_text *);
char * xml_text_dup0(const char *, int);

void xml_node_compile(struct xml_element *);
void xml_element_free(struct xml_element *);
void xml_free(struct xml *);
void xml_init(struct xml *);
void xml_node_print(struct xml_element *, FILE *);
void xml_dump(struct xml *, FILE *);
void xml_text_free(struct xml_text);
void xml_text_move(struct xml_text, const struct xml_text);

struct xml_text xml_text_dup_bool(const int *);
struct xml_text xml_text_dup_double(const double *);
struct xml_text xml_text_dup_float(const float *);
struct xml_text xml_text_dup_uint64(const unsigned long long int *);
struct xml_text xml_text_dup_int64(const long long int *);
struct xml_text xml_text_dup_int32(const int *);
struct xml_text xml_text_dup_uint32(const unsigned int *);
struct xml_text xml_text_dup_string(const char * const *);

struct xml_element * xml_node_lookup(struct xml_element *, const char *, int);
struct xml_element * xml_lookup(struct xml *, const char *, int);
struct xml_element * xml_node_find(struct xml_element *, const char *, int);
struct xml_element * xml_node_attribute(struct xml_element *, const char *, int);
struct xml_element * xml_node_new(const struct xml_element *);
struct xml_element * xml_element_append(struct xml_element *, struct xml_element *);
struct xml_element * xml_element_add(struct xml_element *, struct xml_element *);
struct xml_element * xml_element_init(struct xml_element *, struct xml_element *, enum xml_type,
		enum xml_shape, const char *, unsigned, const char *, unsigned);
struct xml_element * xml_element_init2(struct xml_element *, enum xml_type, enum xml_shape shape,
		const struct xml_text *, const struct xml_text *);
struct xml_element * xml_element_new(enum xml_type, enum xml_shape, const struct xml_text *,
		const struct xml_text *);

struct xml_text xml_text_dup(const char *, int);
struct xml_text xml_text_cat(struct xml_text, const struct xml_text);
struct xml_text xml_text_find_key2(const struct xml_text, const struct xml_text, const char *);
struct xml_text xml_text_unescape(const struct xml_text, char *, int);
struct xml_text xml_text_escape2(const char *, int, char *, int);
struct xml_text xml_node_text(struct xml_element *, char *, int);
struct xml_text xml_text_set(const char *, int);
struct xml_text xml_text_escape(struct xml_text, char *, int);
struct xml_text xml_text_find_key(const char *, const char *, const char *);
struct xml_text xml_text_dup2(const struct xml_text);
struct xml_text xml_text_dup3(const char *);
struct xml_text xml_text_decode(const struct xml_text, char *, int);

#endif // XML_H_
