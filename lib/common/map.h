/*
 * map.h
 *
 *  Created on: Sep 26, 2013
 *      Author: shved
 */

#ifndef MAP_H_
#define MAP_H_

#include <stdarg.h>
#ifdef MAP_WITH_THREAD
#include <pthread.h>
#endif

typedef int (*map_cmp_t)(const void *, const void *, int);
typedef void (*map_del_t)(void *);
typedef void *(*map_cpy_t)(void *, void *, void *);
typedef int (*map_exe_t)(const void *, void *);

typedef enum map_type {

	map_local,
#ifdef MAP_WITH_THREAD
	map_shared,
#endif
} map_type_t;

typedef struct map_node {

	struct map_node *up;
	struct map_node *down;
	void *item;

} map_node_t;

typedef map_node_t * map_iter_t;

typedef struct map_index {

	void *root;
	map_cmp_t cmp;

} map_index_t;

typedef struct map map_base_t;

typedef int (*map_like_t)(map_base_t *, const map_node_t *, const map_node_t *, int);
typedef void (*map_free_t)(map_base_t *, void *);

typedef struct map {

	struct map_node * root;
	struct map_node * tail;
	struct map_index * key;
	int max;
	int size;
	unsigned long long id;
	map_del_t del;
	volatile map_free_t __free;
	volatile map_like_t __like;
#ifdef MAP_WITH_THREAD
	pthread_mutex_t *__mutex;
	pthread_cond_t *__signal;
#endif
} map_t;

typedef enum map_flag {

	map_flag_undefined = 0x00, map_flag_insert = 0x01, map_flag_update = 0x02, map_flag_shift = 0x04,

} map_flag_t;

map_t * map_new(map_del_t del, map_cmp_t cmp, ...);
map_t * map_init(map_t *, map_del_t);
map_t * map_create(map_type_t, map_del_t, map_cmp_t, ...);
map_t * map_map(map_type_t, map_t *, int, map_del_t, map_cmp_t, ...);
map_node_t * map_begin(map_t *);
map_node_t * map_next(map_node_t *);
map_node_t * map_end(map_t *);
map_iter_t * map_iter_begin(map_t *);
map_iter_t * map_iter_end(map_t *);
void map_free(map_t *);
void map_clean(map_t *);
void map_erase(map_t *);
void map_until(map_t *, map_exe_t, void *);
void map_pure(map_t *, void *, int);
int map_push(map_t *, void *);
int map_push_back(map_t *, void *);
void * map_front(map_t *);
void * map_back(map_t *);
void * map_pop(map_t *);
void * map_pop_back(map_t *);
void * map_pop_back_when(map_t *, map_exe_t, void *);
void * map_get(map_t *, void *, int);
void * map_find(map_t *, void *, int);
void * map_find_exec(map_t *, void *, int, map_cpy_t);
void * map_push_find(map_t *, void *, map_cpy_t, void *, map_flag_t *);
void * map_delete(map_t *, map_node_t *);
int map_add_key(map_t *, map_cmp_t);
int map_size(map_t *);
#ifdef MAP_WITH_THREAD
int map_share(map_t *);
void map_signal(map_t *);
void map_broadcast(map_t *);
int map_wait(map_t *);
#endif
int map_walk(map_t *, int, map_exe_t, void *);
void * map_iter_up(map_t *, map_iter_t **);
void * map_iter_down(map_t *, map_iter_t **);
void map_iter_null(map_t *, map_iter_t **);
void map_item_free(void *);
int map_char_cmp(const void *, const void *, int);
int map_item_cmp(const void *, const void *, int);
int map_mem2_cmp(const void *, const void *, int);
int map_mem4_cmp(const void *, const void *, int);
int map_mem8_cmp(const void *, const void *, int);

#endif /* MAP_H_ */
