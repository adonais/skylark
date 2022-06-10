/***************************************************************************
 *
 * Copyright (c) 2016 grab from kernel, Inc. All Rights Reserved
 *
 **************************************************************************/
/*
  Red Black Trees
  (C) 1999  Andrea Arcangeli <andrea@suse.de>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
  linux/include/linux/rbtree.h
  To use rbtrees you'll have to implement your own insert and search cores.
  This will avoid us to use callbacks and to drop drammatically performances.
  I know it's not the cleaner way,  but in C (not in C++) to get
  performances and genericity...
  Some example of insert and search follows here. The search is a plain
  normal search over an ordered tree. The insert instead must be implemented
  in two steps: First, the code must insert the element in order as a red leaf
  in the tree, and then the support library function rb_insert_color() must
  be called. Such function will do the not trivial work to rebalance the
  rbtree, if necessary.
-----------------------------------------------------------------------*/

#ifndef _LINUX_RBTREE_H
#define _LINUX_RBTREE_H

/* for offsetof() */
#include <stddef.h>
/* for uintptr_t */
#include <stdint.h>

#if defined(__GNUC__)
#ifndef container_of
#define container_of(ptr, type, member)                                     \
    __extension__({                                                         \
        const __typeof__(((type *) 0)->member) *__pmember = ((void *) ptr); \
        (type *) ((char *) __pmember - offsetof(type, member));             \
    })
#endif
#endif

#if defined(_MSC_VER)
#if !defined(__cplusplus)
#define inline __inline
#endif
#define container_of(ptr, type, member) ((type *) ((char *) (ptr) - offsetof(type, member)))
#endif

#if (__GUNC__ || __clang__)
#define CACHE_LINE sizeof(intptr_t)
#define RB_NODE_ALIGNED __attribute__((aligned(CACHE_LINE)))
#elif defined(_MSC_VER)
#if defined(_WIN64) || defined(_M_X64)
#define CACHE_LINE 8
#else
#define CACHE_LINE 4
#endif
#define RB_NODE_ALIGNED __declspec(align(CACHE_LINE))
#endif

#define RB_RED 0
#define RB_BLACK 1

struct rb_node
{
    uintptr_t rb_parent_color;
    struct rb_node *rb_right;
    struct rb_node *rb_left;
} RB_NODE_ALIGNED;
/* The alignment might seem pointless, but allegedly CRIS needs it */

struct rb_root
{
    struct rb_node *rb_node;
};

#define rb_parent(r) ((struct rb_node *) ((r)->rb_parent_color & ~3))
#define rb_color(r) ((r)->rb_parent_color & 1)
#define rb_is_red(r) (!rb_color(r))
#define rb_is_black(r) rb_color(r)
#define rb_set_red(r)               \
    do                              \
    {                               \
        (r)->rb_parent_color &= ~1; \
    } while (0)
#define rb_set_black(r)            \
    do                             \
    {                              \
        (r)->rb_parent_color |= 1; \
    } while (0)

#ifdef __cplusplus
extern "C"
{
#endif

static inline void rb_set_parent(struct rb_node *rb, struct rb_node *p)
{
    rb->rb_parent_color = (rb->rb_parent_color & 3) | (uintptr_t) p;
}
static inline void rb_set_color(struct rb_node *rb, int color) { rb->rb_parent_color = (rb->rb_parent_color & ~1) | color; }

#if defined(__GNUC__)
#define RB_ROOT \
    (struct rb_root) { NULL, }
#else
#define RB_ROOT \
    {           \
        0,      \
    }
#endif

#define rb_entry(ptr, type, member) container_of(ptr, type, member)

#define RB_EMPTY_ROOT(root) ((root)->rb_node == NULL)
#define RB_EMPTY_NODE(node) (rb_parent(node) == node)
#define RB_CLEAR_NODE(node) (rb_set_parent(node, node))

static inline void rb_init_node(struct rb_node *rb)
{
    rb->rb_parent_color = 0;
    rb->rb_right = NULL;
    rb->rb_left = NULL;
    RB_CLEAR_NODE(rb);
}

extern void rb_insert_color(struct rb_node *, struct rb_root *);
extern void rb_erase(struct rb_node *, struct rb_root *);

typedef void (*rb_augment_f)(struct rb_node *node, void *data);

extern void rb_augment_insert(struct rb_node *node, rb_augment_f func, void *data);
extern struct rb_node *rb_augment_erase_begin(struct rb_node *node);
extern void rb_augment_erase_end(struct rb_node *node, rb_augment_f func, void *data);

/* Find logical next and previous nodes in a tree */
extern struct rb_node *rb_next(const struct rb_node *);
extern struct rb_node *rb_prev(const struct rb_node *);
extern struct rb_node *rb_first(const struct rb_root *);
extern struct rb_node *rb_last(const struct rb_root *);

/* Fast replacement of a single node without remove/rebalance/add/rebalance */
extern void rb_replace_node(struct rb_node *victim, struct rb_node *new_node, struct rb_root *root);

static inline void rb_link_node(struct rb_node *node, struct rb_node *parent, struct rb_node **rb_link)
{
    node->rb_parent_color = (uintptr_t) parent;
    node->rb_left = node->rb_right = NULL;
    *rb_link = node;
}

#ifdef __cplusplus
}
#endif

#endif /* _LINUX_RBTREE_H */
