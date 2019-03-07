/***************************************************************
  Copyright (c) 2019 ShenZhen Panath Technology, Inc.

  The right to copy, distribute, modify or otherwise make use
  of this software may be licensed only pursuant to the terms
  of an applicable ShenZhen Panath license agreement.
 ***************************************************************/

#ifndef	___RBTREE_H
#define	___RBTREE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct rb_node
{
	unsigned long  rb_parent_color;
#define	RB_RED		0
#define	RB_BLACK	1
	struct rb_node *rb_right;
	struct rb_node *rb_left;
} __attribute__((aligned(sizeof(long))));
    /* The alignment might seem pointless, but allegedly CRIS needs it */

struct rb_root
{
	struct rb_node *rb_node;
};

typedef int (*RB_COMPARE)(struct rb_node *node, void * key);

#define rb_parent(r)   ((struct rb_node *)((r)->rb_parent_color & ~3))
#define rb_color(r)   ((r)->rb_parent_color & 1)
#define rb_is_red(r)   (!rb_color(r))
#define rb_is_black(r) rb_color(r)
#define rb_set_red(r)  do { (r)->rb_parent_color &= ~1; } while (0)
#define rb_set_black(r)  do { (r)->rb_parent_color |= 1; } while (0)

static inline void rb_set_parent(struct rb_node *rb, struct rb_node *p)
{
	rb->rb_parent_color = (rb->rb_parent_color & 3) | (unsigned long)p;
}
static inline void rb_set_color(struct rb_node *rb, int color)
{
	rb->rb_parent_color = (rb->rb_parent_color & ~1) | color;
}

#ifndef offsetof
#define offsetof(type, member) (size_t)&(((type*)0)->member)
#endif

#ifndef container_of
#define container_of(ptr, type, member) ({\
        const typeof( ((type *)0)->member ) *__mptr = (ptr);\
        (type *)( (char *)__mptr - offsetof(type,member) );})
#endif

#define RB_ROOT	(struct rb_root) { NULL, }
#define	rb_entry(ptr, type, member) container_of(ptr, type, member)

#define RB_EMPTY_ROOT(root)	((root)->rb_node == NULL)
#define RB_EMPTY_NODE(node)	(rb_parent(node) == node)
#define RB_CLEAR_NODE(node)	(rb_set_parent(node, node))

static inline void rb_init_node(struct rb_node *rb)
{
	rb->rb_parent_color = 0;
	rb->rb_right = NULL;
	rb->rb_left = NULL;
	RB_CLEAR_NODE(rb);
}

extern void rb_insert_color(struct rb_node *, struct rb_root *);
extern void rb_erase(struct rb_node *, struct rb_root *);

/* Find logical next and previous nodes in a tree */
extern struct rb_node *rb_next(const struct rb_node *);
extern struct rb_node *rb_prev(const struct rb_node *);
extern struct rb_node *rb_first(const struct rb_root *);
extern struct rb_node *rb_last(const struct rb_root *);

/* Fast replacement of a single node without remove/rebalance/add/rebalance */
extern void rb_replace_node(struct rb_node *victim,
			    struct rb_node *new, struct rb_root *root);
extern struct rb_node *rb_delete(struct rb_root *root,
                     void *key, RB_COMPARE compare);
extern struct rb_node *rb_search(struct rb_root *root,
                     void *key, RB_COMPARE compare);
extern int rb_insert(struct rb_root *root, struct rb_node *node,
            void *key, RB_COMPARE compare);

static inline void rb_link_node(struct rb_node * node,
				struct rb_node * parent, struct rb_node ** rb_link)
{
	node->rb_parent_color = (unsigned long )parent;
	node->rb_left = node->rb_right = NULL;

	*rb_link = node;
}

/* rule table operation templet
   The actual table must be in struct of rule_tpl, such as:
   struct rule_tmp1 {
       struct rb_node  tmp1_node;
       uint32_t        tmp1_id;
       uint8_t         val1;
       int             val2;
       ...
   }
*/
struct rule_tpl {
    struct rb_node       node;
    unsigned int          id;
};

typedef void (*TPL_FREE)(struct rb_node *);

/*
  rule templet create function
  root: the rb_root of actual table to be insert.
  id  : the id of actual table
  size: the size of actual table, must be more than sizeof(struct rule_tpl)
 */
static inline void *
rule_tpl_create(struct rb_root *root, unsigned int id, unsigned long size)
{
    struct rule_tpl *tpl;
    struct rb_node **new;
    struct rb_node *parent = NULL;

    if (NULL == root) {
        return NULL;
    }

    new = &(root->rb_node);
    /* Figure out where to put new node */
    while (*new)
    {
        struct rule_tpl *cur = container_of(*new, struct rule_tpl, node);
        parent = *new;
        if (cur->id < id) {
            new = &((*new)->rb_left);
        }
        else if (cur->id > id) {
            new = &((*new)->rb_right);
        }
        else {
            return NULL;
        }
    }

    tpl = (struct rule_tpl *)malloc(size);
    if (NULL == tpl) {
        return NULL;
    }
    memset(tpl, 0, size);

    tpl->id = id;
    /* Add new node and rebalance tree. */
    rb_link_node(&tpl->node, parent, new);
    rb_insert_color(&tpl->node, root);
    return (void *)tpl;
}

/*
  rule templet delete function, release node memory.
  root: the rb_root of actual table to be remove.
  id  : the id of actual table
 */
static inline int
rule_tpl_delete(struct rb_root *root, unsigned int id, TPL_FREE tpl_free)
{
    struct rb_node *node;
    if (NULL == root) {
        return -1;
    }

    node = root->rb_node;
    while (node != NULL) {
        struct rule_tpl *cur = container_of(node, struct rule_tpl, node);
        if (cur->id < id) {
            node = node->rb_left;
        }
        else if (cur->id > id) {
            node = node->rb_right;
        }
        else {
            break;
        }
    }

    if (node != NULL) {
        rb_erase(node, root);
        if (tpl_free) {
            tpl_free(node);
        }
        free((void *)node);
        return 0;
    }

    return -1;
}

/*
  rule templet node clear recursive function,
  it will delete the sub tree of the node,
  if node is point to root, it will delete the whole tree
  node: the any rb_node of rb_tree
  tpl_free: the free function, if there are some resources to release
 */
static inline void
rule_tpl_node_clear(struct rb_node *node, TPL_FREE tpl_free)
{
    if (node) {
        rule_tpl_node_clear(node->rb_left, tpl_free);
        rule_tpl_node_clear(node->rb_right, tpl_free);
        if (tpl_free) {
            tpl_free(node);
        }
        free((void *)node);
    }
}

/*
  rule templet tree clear function,
  it will delete the whole tree
  root: the rb_root of rb_tree
  tpl_free: the free function, if there are some resources to release
 */
static inline int
rule_tpl_tree_clear(struct rb_root *root, TPL_FREE tpl_free)
{
    if (NULL == root) {
        return -1;
    }

    rule_tpl_node_clear(root->rb_node, tpl_free);
    return 0;
}

/*
  rule templet search function
  root: the rb_root of actual table to be insert.
  id  : the id of actual table
 */
static inline void *
rule_tpl_search(struct rb_root *root, unsigned int id) {
    struct rb_node *node;
    if (NULL == root) {
        return NULL;
    }

    node = root->rb_node;
    while (node != NULL) {
        struct rule_tpl *cur = container_of(node, struct rule_tpl, node);
        if (cur->id < id) {
            node = node->rb_left;
        }
        else if (cur->id > id) {
            node = node->rb_right;
        }
        else {
            return (void *)node;
        }
    }
    return NULL;
}

#endif	/* _LINUX_RBTREE_H */
