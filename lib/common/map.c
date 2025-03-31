/*
 * map.c
 *
 *  Created on: Sep 26, 2013
 *      Author: shved
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "map.h"

typedef enum {
    preorder,
    postorder,
    endorder,
    leaf
} VISIT;

typedef int (*__compar_d_fn_t)(const void *, const void *, const void *);

typedef int (*__action_fn_t)(const void *, VISIT, int, const void *);

typedef void (*__free_fn_t)(void *);

typedef struct node_t {
    /* Callers expect this to be the first element in the structure - do not move!  */
    const void *key;
    struct node_t *left;
    struct node_t *right;
    unsigned int red: 1;
} *node;

typedef const struct node_t *const_node;

/*
 * Possibly "split" a node with two red successors, and/or fix up two red
 * edges in a row.  ROOTP is a pointer to the lowest node we visited, PARENTP
 * and GPARENTP pointers to its parent/grandparent.  P_R and GP_R contain the
 * comparison values that determined which way was taken in the tree to reach ROOTP.
 * MODE is 1 if we need not do the split, but must check for two red edges between
 * GPARENTP and ROOTP.
 */
static void maybe_split_for_insert(node *rootp, node *parentp, node *gparentp, int p_r, int gp_r, int mode) {
    node root = *rootp;
    node *rp, *lp;
    rp = &(*rootp)->right;
    lp = &(*rootp)->left;

    /* See if we have to split this node (both successors red).  */
    if (mode == 1 || ((*rp) != NULL && (*lp) != NULL && (*rp)->red && (*lp)->red)) {
        /* This node becomes red, its successors black.  */
        root->red = 1;
        if (*rp)
            (*rp)->red = 0;
        if (*lp)
            (*lp)->red = 0;

        /* If the parent of this node is also red, we have to do rotations.  */
        if (parentp != NULL && (*parentp)->red) {
            node gp = *gparentp;
            node p = *parentp;
            /*
             There are two main cases:
             1. The edge types (left or right) of the two red edges differ.
             2. Both red edges are of the same type.
             There exist two symmetries of each case, so there is a total of
             4 cases.
             */
            if ((p_r > 0) != (gp_r > 0)) {
                /*
                 Put the child at the top of the tree, with its parent
                 and grandparent as successors.
                 */
                p->red = 1;
                gp->red = 1;
                root->red = 0;
                if (p_r < 0) {
                    /* Child is left of parent.  */
                    p->left = *rp;
                    *rp = p;
                    gp->right = *lp;
                    *lp = gp;
                } else {
                    /* Child is right of parent.  */
                    p->right = *lp;
                    *lp = p;
                    gp->left = *rp;
                    *rp = gp;
                }
                *gparentp = root;
            } else {
                *gparentp = *parentp;
                /*
                 Parent becomes the top of the tree, grandparent and
                 child are its successors.
                 */
                p->red = 0;
                gp->red = 1;
                if (p_r < 0) {
                    /* Left edges.  */
                    gp->left = p->right;
                    p->right = gp;
                } else {
                    /* Right edges.  */
                    gp->right = p->left;
                    p->left = gp;
                }
            }
        }
    }
}

/*
 * Find or insert datum into search tree.
 * KEY is the key to be located, ROOTP is the address of tree root,
 * COMPAR the ordering function.
 */
static void *tsearch2(const void *key, void **vrootp, __compar_d_fn_t compar, const void *arg) {
    node q;
    node *parentp = NULL, *gparentp = NULL;
    node *rootp = (node *) vrootp;
    node *nextp;
    int r = 0, p_r = 0, gp_r = 0; /* No they might not, Mr Compiler.  */

    if (rootp == NULL)
        return NULL;

    /* This saves some additional tests below.  */
    if (*rootp != NULL)
        (*rootp)->red = 0;

    nextp = rootp;
    while (*nextp != NULL) {
        node root = *rootp;
        r = (*compar)(key, root->key, arg);
        if (r == 0)
            return root;

        maybe_split_for_insert(rootp, parentp, gparentp, p_r, gp_r, 0);
        /*
         If that did any rotations, parentp and gparentp are now garbage.
         That doesn't matter, because the values they contain are never
         used again in that case.
         */
        nextp = r < 0 ? &root->left : &root->right;
        if (*nextp == NULL)
            break;

        gparentp = parentp;
        parentp = rootp;
        rootp = nextp;

        gp_r = p_r;
        p_r = r;
    }

    q = (struct node_t *) malloc(sizeof(struct node_t));
    if (q != NULL) {
        *nextp = q; /* link new node to old */
        q->key = key; /* initialize new node */
        q->red = 1;
        q->left = q->right = NULL;

        if (nextp != rootp)
            /*
             There may be two red edges in a row now, which we must avoid by
             rotating the tree.
             */
            maybe_split_for_insert(nextp, rootp, parentp, r, p_r, 1);
    }

    return q;
}

/*
 * Find datum in search tree.
 * KEY is the key to be located, ROOTP is the address of tree root,
 * COMPAR the ordering function.
 */
static void *tfind2(const void *key, void *const *vrootp, __compar_d_fn_t compar, const void *arg) {
    node *rootp = (node *) vrootp;

    if (rootp == NULL)
        return NULL;

    while (*rootp != NULL) {
        node root = *rootp;
        int r;

        r = (*compar)(key, root->key, arg);
        if (r == 0)
            return root;

        rootp = r < 0 ? &root->left : &root->right;
    }
    return NULL;
}

/*
 * Delete node with given key.
 * KEY is the key to be deleted, ROOTP is the address of the root of tree,
 * COMPAR the comparison function.
 */
static void *tdelete2(const void *key, void **vrootp, __compar_d_fn_t compar, const void *arg) {
    node p, q, r, retval;
    int cmp;
    node *rootp = (node *) vrootp;
    node root, unchained;

    /*
     Stack of nodes so we remember the parents without recursion.  It's
     VERY unlikely that there are paths longer than 40 nodes.  The tree
     would need to have around 250.000 nodes.
     */
    int stacksize = 100;
    int sp = 0;
    node *nodestack[100];

    if (rootp == NULL)
        return NULL;
    p = *rootp;
    if (p == NULL)
        return NULL;

    while ((cmp = (*compar)(key, (*rootp)->key, arg)) != 0) {
        if (sp == stacksize)
            abort();

        nodestack[sp++] = rootp;
        p = *rootp;
        rootp = ((cmp < 0) ? &(*rootp)->left : &(*rootp)->right);
        if (*rootp == NULL)
            return NULL;
    }

    /*
     This is bogus if the node to be deleted is the root... this routine
     really should return an integer with 0 for success, -1 for failure
     and errno = ESRCH or something.
     */
    retval = p;

    /*
     We don't unchain the node we want to delete. Instead, we overwrite
     it with its successor and unchain the successor.  If there is no
     successor, we really unchain the node to be deleted.
     */
    root = *rootp;

    r = root->right;
    q = root->left;

    if (q == NULL || r == NULL)
        unchained = root;
    else {
        node *parent = rootp, *up = &root->right;
        for (;;) {
            if (sp == stacksize)
                abort();
            nodestack[sp++] = parent;
            parent = up;
            if ((*up)->left == NULL)
                break;
            up = &(*up)->left;
        }
        unchained = *up;
    }

    /*
     We know that either the left or right successor of UNCHAINED is NULL.
     R becomes the other one, it is chained into the parent of UNCHAINED.
     */
    r = unchained->left;
    if (r == NULL)
        r = unchained->right;
    if (sp == 0)
        *rootp = r;
    else {
        q = *nodestack[sp - 1];
        if (unchained == q->right)
            q->right = r;
        else
            q->left = r;
    }

    if (unchained != root)
        root->key = unchained->key;
    if (!unchained->red) {
        /*
         Now we lost a black edge, which means that the number of black
         edges on every path is no longer constant.  We must balance the
         tree.
         NODESTACK now contains all parents of R.  R is likely to be NULL
         in the first iteration.
         NULL nodes are considered black throughout - this is necessary for
         correctness.
         */
        while (sp > 0 && (r == NULL || !r->red)) {
            node *pp = nodestack[sp - 1];
            p = *pp;
            /* Two symmetric cases.  */
            if (r == p->left) {
                /*
                 Q is R's brother, P is R's parent.  The subtree with root
                 R has one black edge less than the subtree with root Q.
                 */
                q = p->right;
                if (q->red) {
                    /*
                     If Q is red, we know that P is black. We rotate P left
                     so that Q becomes the top node in the tree, with P below
                     it.  P is colored red, Q is colored black.
                     This action does not change the black edge count for any
                     leaf in the tree, but we will be able to recognize one
                     of the following situations, which all require that Q
                     is black.
                     */
                    q->red = 0;
                    p->red = 1;
                    /* Left rotate p.  */
                    p->right = q->left;
                    q->left = p;
                    *pp = q;
                    /* Make sure pp is right if the case below tries to use
                     it.  */
                    nodestack[sp++] = pp = &q->left;
                    q = p->right;
                }
                /*
                 We know that Q can't be NULL here.  We also know that Q is
                 black.
                 */
                if ((q->left == NULL || !q->left->red) && (q->right == NULL || !q->right->red)) {
                    /*
                     Q has two black successors.  We can simply color Q red.
                     The whole subtree with root P is now missing one black
                     edge.  Note that this action can temporarily make the
                     tree invalid (if P is red).  But we will exit the loop
                     in that case and set P black, which both makes the tree
                     valid and also makes the black edge count come out
                     right.  If P is black, we are at least one step closer
                     to the root and we'll try again the next iteration.
                     */
                    q->red = 1;
                    r = p;
                } else {
                    /*
                     Q is black, one of Q's successors is red.  We can
                     repair the tree with one operation and will exit the
                     loop afterwards.
                     */
                    if (q->right == NULL || !q->right->red) {
                        /*
                         The left one is red.  We perform the same action as
                         in maybe_split_for_insert where two red edges are
                         adjacent but point in different directions:
                         Q's left successor (let's call it Q2) becomes the
                         top of the subtree we are looking at, its parent (Q)
                         and grandparent (P) become its successors. The former
                         successors of Q2 are placed below P and Q.
                         P becomes black, and Q2 gets the color that P had.
                         This changes the black edge count only for node R and
                         its successors.
                         */
                        node q2 = q->left;
                        q2->red = p->red;
                        p->right = q2->left;
                        q->left = q2->right;
                        q2->right = q;
                        q2->left = p;
                        *pp = q2;
                        p->red = 0;
                    } else {
                        /*
                         It's the right one.  Rotate P left. P becomes black,
                         and Q gets the color that P had.  Q's right successor
                         also becomes black.  This changes the black edge
                         count only for node R and its successors.
                         */
                        q->red = p->red;
                        p->red = 0;

                        q->right->red = 0;

                        /* left rotate p */
                        p->right = q->left;
                        q->left = p;
                        *pp = q;
                    }

                    /* We're done.  */
                    sp = 1;
                    r = NULL;
                }
            } else {
                /* Comments: see above.  */
                q = p->left;
                if (q->red) {
                    q->red = 0;
                    p->red = 1;
                    p->left = q->right;
                    q->right = p;
                    *pp = q;
                    nodestack[sp++] = pp = &q->right;
                    q = p->left;
                }
                if ((q->right == NULL || !q->right->red) && (q->left == NULL || !q->left->red)) {
                    q->red = 1;
                    r = p;
                } else {
                    if (q->left == NULL || !q->left->red) {
                        node q2 = q->right;
                        q2->red = p->red;
                        p->left = q2->right;
                        q->right = q2->left;
                        q2->left = q;
                        q2->right = p;
                        *pp = q2;
                        p->red = 0;
                    } else {
                        q->red = p->red;
                        p->red = 0;
                        q->left->red = 0;
                        p->left = q->right;
                        q->right = p;
                        *pp = q;
                    }
                    sp = 1;
                    r = NULL;
                }
            }
            --sp;
        }
        if (r != NULL)
            r->red = 0;
    }

    free(unchained);
    return retval;
}

/*
 * Walk the nodes of a tree.
 * ROOT is the root of the tree to be walked, ACTION the function to be called at each node.
 * LEVEL is the level of ROOT in the whole tree.
 */
static int trecurse(const void *vroot, __action_fn_t action, int level, const void *arg) {
    int r;
    const_node root = (const_node) vroot;

    if (root->left == NULL && root->right == NULL) {
        r = (*action)(root, leaf, level, arg);
        if (r)
            return r;
    } else {
        r = (*action)(root, preorder, level, arg);
        if (r)
            return r;
        if (root->left != NULL) {
            r = trecurse(root->left, action, level + 1, arg);
            if (r)
                return r;
        }
        r = (*action)(root, postorder, level, arg);
        if (r)
            return r;
        if (root->right != NULL) {
            r = trecurse(root->right, action, level + 1, arg);
            if (r)
                return r;
        }
        r = (*action)(root, endorder, level, arg);
        if (r)
            return r;
    }

    return r;
}

/*
 * Walk the nodes of a tree.
 * ROOT is the root of the tree to be walked, ACTION the function to be called at each node.
 */
static int twalk2(const void *vroot, __action_fn_t action, const void *arg) {
    const_node root = (const_node) vroot;

    if (root != NULL && action != NULL)
        return trecurse(root, action, 0, arg);
    return 0;
}

/*
 * The standardized functions miss an important functionality: the tree cannot be removed easily.
 * We provide a function to do this.
 */
static void tdestroy_recurse(node root, __free_fn_t freefct) {
    if (root->left != NULL)
        tdestroy_recurse(root->left, freefct);
    if (root->right != NULL)
        tdestroy_recurse(root->right, freefct);
    (*freefct)((void *) root->key);
    /* Free the node itself.  */
    free(root);
}

/**
 *
 */
static void tdestroy2(void *proot, __free_fn_t free_fn) {
    const node root = proot;

    if (root != NULL)
        tdestroy_recurse(root, free_fn);
}

#ifdef MAP_WITH_THREAD
#define MAP_LOCK_ACQUIRE(m)             do { map_lock(m); } while(0)
#define MAP_LOCK_RESTORE(m)             do { map_unlock(m); } while(0)
#define MAP_LOCK_DESTROY(m)             do { map_destroy(m); } while(0)
#else
#define MAP_LOCK_ACQUIRE(m)
#define MAP_LOCK_RESTORE(m)
#define MAP_LOCK_DESTROY(m)
#endif
#define MAP_LOCK_RELEASE(m, type, x)    do { type X = x; MAP_LOCK_RESTORE(m); return X; } while(0)
#define MAP_NODE_FAILURE                ((void *) -1)

/*
 *
 */
void map_item_free(void *item) {
    free(item);
}

/**
 *
 */
int map_char_cmp(const void *a, const void *b, int i) {
    return strcmp(a, b);
}

/**
 *
 */
int map_item_cmp(const void *a, const void *b, int i) {
    return memcmp(&a, &b, sizeof(void *));
}

/**
 *
 */
int map_mem8_cmp(const void *a, const void *b, int i) {
    return memcmp(a, b, 8);
}

/**
 *
 */
int map_mem4_cmp(const void *a, const void *b, int i) {
    return memcmp(a, b, 4);
}

/**
 *
 */
int map_mem2_cmp(const void *a, const void *b, int i) {
    return memcmp(a, b, 2);
}

/**
 *
 */
static int map_internal_cmp(map_t *map, const map_node_t *a, const map_node_t *b, const int i) {
    return map->key[i].cmp(a->item, b->item, i);
}

/**
 *
 */
static void map_internal_del(map_t *map, void *a) {
    if (map->del)
        map->del(a);
}

#ifdef MAP_WITH_THREAD

/**
 *
 */
static void map_lock(map_t *map) {
    if (map)
        if (map->__mutex)
            assert(!pthread_mutex_lock(map->__mutex));
}

/**
 *
 */
static void map_unlock(map_t *map) {
    if (map)
        if (map->__mutex)
            assert(!pthread_mutex_unlock(map->__mutex));
}

/**
 *
 */
int map_share(map_t *map) {
    if (map) {
        map->__mutex = calloc(1, sizeof(pthread_mutex_t));
        map->__signal = calloc(1, sizeof(pthread_cond_t));
        if (map->__mutex && map->__signal && !pthread_mutex_init(map->__mutex, 0)
            && !pthread_cond_init(map->__signal, 0)) {
            return 0;
            }
    }
    return -1;
}

/**
 *
 */
void map_signal(map_t *map) {
    if (map)
        if (map->__signal)
            assert(!pthread_cond_signal(map->__signal));
}

/**
 *
 */
void map_broadcast(map_t *map) {
    if (map)
        if (map->__signal)
            assert(!pthread_cond_broadcast(map->__signal));
}

/**
 *
 */
int map_wait(map_t *map) {
    if (!map || !map->__mutex || !map->__signal)
        return -1;

    assert(!pthread_mutex_lock(map->__mutex));

    assert(!pthread_cond_wait(map->__signal, map->__mutex));

    assert(!pthread_mutex_unlock(map->__mutex));

    return 0;
}

/**
 *
 * @param map
 */
static void map_destroy(map_t *map) {
    if (map->__mutex)
        pthread_mutex_destroy(map->__mutex);
    if (map->__signal)
        pthread_cond_destroy(map->__signal);

    free(map->__mutex);
    free(map->__signal);
}

#endif

/**
 *
 */
map_node_t *map_begin(map_t *map) {
    if (!map)
        return 0;

    return map->root;
}

/**
 *
 */
map_node_t *map_next(map_node_t *node) {
    if (node)
        return node->down;

    return 0;
}

/**
 *
 */
map_node_t *map_end(map_t *map) {
    if (!map)
        return 0;

    return map->tail;
}

/**
 *
 */
map_t *map_init(map_t *map, const map_del_t del) {
    memset(map, 0, sizeof(map_t));

    map->__free = map_internal_del;
    map->__like = map_internal_cmp;

    map->del = del;

    return map;
}

/**
 *
 */
static map_t *map_create2(const map_type_t type, const map_del_t del, map_cmp_t cmp, const va_list ap) {
    map_t *map = calloc(1, sizeof(map_t));

    if (map) {
#ifdef MAP_WITH_THREAD
        if (type == map_shared && map_share(map)) {
            map_free(map);
            return NULL;
        }
#endif

        map->__free = map_internal_del;
        map->__like = map_internal_cmp;

        while (cmp) {
            struct map_index *key = realloc(map->key, (map->max + 1) * sizeof(map_index_t));
            if (!key) {
                map_free(map);
                return NULL;
            }
            map->key = key;
            map->key[map->max].cmp = cmp;
            map->key[map->max].root = 0;

            cmp = va_arg(ap, map_cmp_t);

            map->max++;
        }
        map->del = del;
    }

    return map;
}

/**
 *
 */
map_t *map_create(const map_type_t type, const map_del_t del, const map_cmp_t cmp, ...) {
    va_list ap;
    va_start(ap, cmp);
    map_t *map = map_create2(type, del, cmp, ap);
    va_end(ap);

    return map;
}

/**
 *
 */
map_t *map_new(const map_del_t del, const map_cmp_t cmp, ...) {
    va_list ap;
    va_start(ap, cmp);
    map_t *map = map_create2(map_local, del, cmp, ap);
    va_end(ap);

    return map;
}

/**
 *
 */
typedef struct map_iterator {
    const map_exe_t exe;
    void *foo;
} map_iterator_t;

/**
 *
 * @param node
 * @param which
 * @param deep
 * @param arg
 * @return
 */
int map_internal_action(const void *node, VISIT which, int deep, const void *arg) {
    const struct map_iterator *iter = arg;
    switch (which) {
        case leaf:
        case postorder:
            return iter->exe(*(const map_node_t **) node, iter->foo);
        default:
            return 0;
    }
}

/**
 *
 */
int map_walk(map_t *map, const int index, const map_exe_t exe, void *foo) {
    MAP_LOCK_ACQUIRE(map);

    if (map->max > index) {
        map_iterator_t iter = {.exe = exe, .foo = foo};

        int fail = twalk2(map->key[index].root, map_internal_action, &iter);

        MAP_LOCK_RELEASE(map, int, fail);
    }
    MAP_LOCK_RELEASE(map, int, -1);
}

/**
 *
 */
static int map_item_push(const void *node, void *map) {
    return map_push(map, ((const map_node_t *) node)->item);
}

/**
 *
 */
map_t *map_map(const map_type_t type, map_t *old, const int index, const map_del_t del, const map_cmp_t cmp, ...) {
    va_list ap;
    va_start(ap, cmp);
    map_t *map = map_create2(type, del, cmp, ap);
    va_end(ap);
    if (map_walk(old, index, map_item_push, map)) {
        old = map;
        map = 0;
    }
    while (map_pop(old)) {
    }
    map_free(old);
    return map;
}

/**
 *
 */
typedef struct map_compare {
    map_t *map;
    int index;
} map_compare_t;

/**
 *
 * @param a
 * @param b
 * @param p
 * @return
 */
static int map_internal_cmp2(const void *a, const void *b, const void *p) {
    const struct map_compare *q = p;
    return q->map->__like(q->map, a, b, q->index);
}

/**
 *
 */
static void *map_remove(map_t *map, map_node_t *node) {
    if (!map || !node)
        return 0;

    if (node->up)
        node->up->down = node->down;
    else
        map->root = node->down;

    if (node->down)
        node->down->up = node->up;
    else
        map->tail = node->up;

    int index = 0;

    while (map->max > index) {
        map_compare_t cmp = {.map = map, .index = index};

        tdelete2(node, &map->key[index].root, map_internal_cmp2, &cmp);

        index++;
    }
    void *item = node->item;

    free(node);

    map->size--;

    return item;
}

/**
 *
 */
void *map_delete(map_t *map, map_node_t *node) {
    MAP_LOCK_ACQUIRE(map);

    MAP_LOCK_RELEASE(map, void *, map_remove(map, node));
}

/**
 *
 */
void map_erase(map_t *map) {
    MAP_LOCK_ACQUIRE(map);

    while (map && map->root)
        if (map->__free)
            map->__free(map, map_remove(map, map->root));
        else
            map_remove(map, map->root);

    MAP_LOCK_RESTORE(map);
}

/**
 *
 */
void map_until(map_t *map, const map_exe_t yes, void *exp) {
    MAP_LOCK_ACQUIRE(map);

    while (map && map->tail && yes(map->tail->item, exp))
        if (map->__free)
            map->__free(map, map_remove(map, map->tail));
        else
            map_remove(map, map->tail);

    MAP_LOCK_RESTORE(map);
}

/**
 *
 */
void map_clean(map_t *map) {
    if (map) {
        map_erase(map);
        MAP_LOCK_DESTROY(map);
        free(map->key);
    }
}

/**
 *
 */
void map_free(map_t *map) {
    if (map) {
        map_clean(map);
        free(map);
    }
}

/**
 *
 */
static map_node_t *map_key(map_t *map, map_node_t *node, const int index) {
    if (map->max > index) {
        map_compare_t cmp = {.map = map, .index = index};

        void **leaf = tsearch2(node, &map->key[index].root, map_internal_cmp2, &cmp);

        if (!leaf)
            return MAP_NODE_FAILURE;

        if (*leaf != node)
            return *leaf;

        if (map_key(map, node, index + 1)) {
            tdelete2(node, &map->key[index].root, map_internal_cmp2, &cmp);
            return MAP_NODE_FAILURE;
        }
    }

    return 0;
}

/**
 *
 */
static map_node_t *map_index(map_t *map, map_node_t *node) {
    return map_key(map, node, 0);
}

/**
 *
 */
int map_add_key(map_t *map, const map_cmp_t cmp) {
    if (!map || !cmp)
        return -1;

    MAP_LOCK_ACQUIRE(map);

    map_index_t *key = realloc(map->key, (map->max + 1) * sizeof(map_index_t));
    if (!key)
        MAP_LOCK_RELEASE(map, int, -1);

    map->key = key;
    map->key[map->max].cmp = cmp;
    map->key[map->max].root = 0;
    map->max++;

    map_node_t *node = map->root;
    while (node) {
        if (map_key(map, node, map->max - 1)) {
            map->max--;
            tdestroy2(&map->key[map->max].root, 0);
            MAP_LOCK_RELEASE(map, int, -1);
        }
        node = node->down;
    }
    MAP_LOCK_RELEASE(map, int, 0);
}

/**
 *
 */
int map_push_back(map_t *map, void *item) {
    MAP_LOCK_ACQUIRE(map);

    if (map && item) {
        map_node_t *node = calloc(1, sizeof(map_node_t));
        if (node) {
            node->item = item;
            node->up = map->tail;

            if (map_index(map, node) == 0) {
                if (map->tail)
                    map->tail->down = node;
                else
                    map->root = node;

                map->tail = node;
                map->size++;

                MAP_LOCK_RELEASE(map, int, 0);
            }
            free(node);
        }
    }

    MAP_LOCK_RELEASE(map, int, -1);
}

/**
 *
 */
void *map_push_find(map_t *map, void *item, const map_cpy_t cpy, void *arg, map_flag_t *flag) {
    MAP_LOCK_ACQUIRE(map);

    if (map && item) {
        map_node_t *node = calloc(1, sizeof(map_node_t));
        if (node) {
            node->item = item;
            node->down = map->root;

            map_node_t *find = map_index(map, node);
            if (find == 0) {
                /*
                 * insert
                 */
                if (cpy)
                    node->item = cpy(0, item, arg);

                if (node->item) {
                    if (map->root)
                        map->root->up = node;
                    else
                        map->tail = node;

                    map->root = node;
                    map->size++;

                    if (flag)
                        flag[0] &= map_flag_insert;

                    MAP_LOCK_RELEASE(map, void *, node->item);
                }
            } else if (find != MAP_NODE_FAILURE) {
                if (flag) {
                    /*
                     * update
                     */
                    if (flag[0] & map_flag_update)
                        if (cpy)
                            cpy(find->item, item, arg);
                    /*
                     * shift up
                     */
                    if (flag[0] & map_flag_shift) {
                        if (find->up)
                            find->up->down = find->down;
                        else
                            map->root = find->down;
                        if (find->down)
                            find->down->up = find->up;
                        else
                            map->tail = find->up;
                        if (map->root)
                            map->root->up = find;
                        else
                            map->tail = find;
                        find->down = map->root;
                        find->up = 0;
                        map->root = find;
                    }
                }
                free(node);

                MAP_LOCK_RELEASE(map, void *, find->item);
            }
            free(node);
        }
    }

    MAP_LOCK_RELEASE(map, void *, 0);
}

/**
 *
 */
int map_push(map_t *map, void *item) {
    MAP_LOCK_ACQUIRE(map);

    if (map && item) {
        map_node_t *node = calloc(1, sizeof(map_node_t));
        if (node) {
            node->item = item;
            node->down = map->root;

            if (map_index(map, node) == 0) {
                if (map->root)
                    map->root->up = node;
                else
                    map->tail = node;

                map->root = node;
                map->size++;

                MAP_LOCK_RELEASE(map, int, 0);
            }
            free(node);
        }
    }

    MAP_LOCK_RELEASE(map, int, -1);
}

/**
 *
 */
void *map_front(map_t *map) {
    MAP_LOCK_ACQUIRE(map);

    if (map)
        if (map->root)
            MAP_LOCK_RELEASE(map, void *, map->root->item);

    MAP_LOCK_RELEASE(map, void *, 0);
}

/**
 *
 */
void *map_back(map_t *map) {
    MAP_LOCK_ACQUIRE(map);

    if (map)
        if (map->tail)
            MAP_LOCK_RELEASE(map, void *, map->tail->item);

    MAP_LOCK_RELEASE(map, void *, 0);
}

/**
 *
 */
void *map_pop(map_t *map) {
    MAP_LOCK_ACQUIRE(map);

    if (map)
        MAP_LOCK_RELEASE(map, void *, map_remove(map, map->root));

    MAP_LOCK_RELEASE(map, void *, 0);
}

/**
 *
 */
void *map_pop_back(map_t *map) {
    MAP_LOCK_ACQUIRE(map);

    if (map)
        MAP_LOCK_RELEASE(map, void *, map_remove(map, map->tail));

    MAP_LOCK_RELEASE(map, void *, 0);
}

/**
 *
 */
void *map_pop_back_when(map_t *map, const map_exe_t yes, void *foo) {
    MAP_LOCK_ACQUIRE(map);

    if (map) {
        map_node_t *node = map->tail;
        while (node)
            if (yes(node->item, foo))
                MAP_LOCK_RELEASE(map, void *, map_remove(map, node));
            else
                node = node->up;
    }
    MAP_LOCK_RELEASE(map, void *, 0);
}

/**
 *
 */
static map_node_t *map_search(map_t *map, void *item, const int index) {
    if (map && index < map->max) {
        map_node_t node = {.item = item};

        map_compare_t cmp = {.map = map, .index = index};

        map_node_t **leaf = tfind2(&node, &map->key[index].root, map_internal_cmp2, &cmp);
        if (leaf)
            return *leaf;
    }
    return 0;
}

/**
 *
 */
void map_pure(map_t *map, void *item, const int index) {
    MAP_LOCK_ACQUIRE(map);

    map_node_t *node = map_search(map, item, index);
    if (node) {
        if (map->__free)
            map->__free(map, map_remove(map, node));
        else
            map_remove(map, node);
    }
    MAP_LOCK_RESTORE(map);
}

/**
 *
 */
void *map_get(map_t *map, void *item, const int index) {
    MAP_LOCK_ACQUIRE(map);

    map_node_t *node = map_search(map, item, index);
    if (node)
        MAP_LOCK_RELEASE(map, void *, map_remove(map, node));

    MAP_LOCK_RELEASE(map, void *, 0);
}

/**
 *
 */
void *map_find_exec(map_t *map, void *item, const int index, const map_cpy_t exe) {
    MAP_LOCK_ACQUIRE(map);

    const map_node_t *node = map_search(map, item, index);
    if (node)
        MAP_LOCK_RELEASE(map, void *, exe(node->item, 0, 0));

    MAP_LOCK_RELEASE(map, void *, 0);
}

/**
 *
 */
void *map_find(map_t *map, void *item, const int index) {
    MAP_LOCK_ACQUIRE(map);

    const map_node_t *node = map_search(map, item, index);
    if (node)
        MAP_LOCK_RELEASE(map, void *, node->item);

    MAP_LOCK_RELEASE(map, void *, 0);
}

/**
 *
 */
int map_size(map_t *map) {
    MAP_LOCK_ACQUIRE(map);

    if (map)
        MAP_LOCK_RELEASE(map, int, map->size);

    MAP_LOCK_RELEASE(map, int, 0);
}

/**
 *
 */
void *map_iter_down(map_t *map, map_iter_t **iter) {
    MAP_LOCK_ACQUIRE(map);

    if (map && iter && *iter && **iter)
        MAP_LOCK_RELEASE(map, void *, (**iter)->item;
                     *iter = &(**iter)->down
    )
        ;

    MAP_LOCK_RELEASE(map, void *, 0);
}

/**
 *
 */
void *map_iter_up(map_t *map, map_iter_t **iter) {
    MAP_LOCK_ACQUIRE(map);

    if (map && iter && *iter && **iter)
        MAP_LOCK_RELEASE(map, void *, (**iter)->item;
                     *iter = &(**iter)->up
    )
        ;

    MAP_LOCK_RELEASE(map, void *, 0);
}

/**
 *
 */
void map_iter_null(map_t *map, map_iter_t **iter) {
    MAP_LOCK_ACQUIRE(map);

    if (map && iter && *iter)
        *iter = 0;

    MAP_LOCK_RESTORE(map);
}

/**
 *
 */
map_iter_t *map_iter_begin(map_t *map) {
    MAP_LOCK_ACQUIRE(map);

    if (map)
        MAP_LOCK_RELEASE(map, map_iter_t *, &map->root);

    MAP_LOCK_RELEASE(map, void *, 0);
}

/**
 *
 */
map_iter_t *map_iter_end(map_t *map) {
    MAP_LOCK_ACQUIRE(map);

    if (map)
        MAP_LOCK_RELEASE(map, map_iter_t *, &map->tail);

    MAP_LOCK_RELEASE(map, void *, 0);
}
