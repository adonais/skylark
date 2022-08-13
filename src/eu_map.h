/******************************************************************************
 * This file is part of Skylark project
 * Copyright ©2022 Hua andy <hua.andy@gmail.com>

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * at your option any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *******************************************************************************/

#ifndef _EU_MAP_H_
#define _EU_MAP_H_

#include "eu_rbtree.h"
#include <stdio.h>
#include <stdlib.h>
#if defined(__GNUC__)
#include <unistd.h>
#endif
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

struct map
{
    char *key;
    char *val;
    struct rb_node node;
} RB_NODE_ALIGNED;

struct acshow
{
    char *str;
    struct rb_node node;
} RB_NODE_ALIGNED;

typedef struct map map_t;
typedef struct rb_root root_t;
typedef struct rb_node rb_node_t;
typedef struct acshow acshow_t;

map_t *map_get(root_t *root, const char *key);
int map_put(root_t *root, const char *key, const char *val);
map_t *map_first(root_t *tree);
map_t *map_last(root_t *tree);
map_t *map_next(rb_node_t *node);
map_t *map_prev(rb_node_t *node);
void map_free(map_t *node);
void map_destory(root_t *root);

int ac_get(root_t *root, const char *val, char *buf, int buf_len);
int ac_put(root_t *root, const char *str);
acshow_t *ac_first(root_t *tree);
acshow_t *ac_last(root_t *tree);
acshow_t *ac_next(rb_node_t *node);
acshow_t *ac_prev(rb_node_t *node);
void ac_free(acshow_t *node);
void ac_destory(root_t *root);

#ifdef __cplusplus
}
#endif

#endif // EU_MAP_H_
