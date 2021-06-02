/******************************************************************************
 * This file is part of Skylark project
 * Copyright ©2021 Hua andy <hua.andy@gmail.com>

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

#include "eu_map.h"

#define VALUE_LEN 4096

#ifndef ANY_WORD
#define ANY_WORD  "///*///"
#endif

map_t *
map_get(root_t *root, const char *str)
{
    rb_node_t *node = root->rb_node;
    while (node)
    {
        map_t *data = rb_entry(node, map_t, node);
        int cmp = strcmp(str, data->key);
        if (cmp < 0)
        {
            node = node->rb_left;
        }
        else if (cmp > 0)
        {
            node = node->rb_right;
        }
        else
        {
            return data;
        }
    }
    return NULL;
}

int
map_put(root_t *root, const char *key, const char *val)
{
    map_t *data = (map_t *) malloc(sizeof(map_t));
    if (!data)
    {
        return 1;
    }
    data->key = (char *) malloc((strlen(key) + 1) * sizeof(char));
    if (!data->key)
    {
        free(data);
        return 1;
    }
    strcpy(data->key, key);
    data->val = (char *) malloc((strlen(val) + 1) * sizeof(char));
    if (!data->val)
    {
        free(data->key);
        free(data);
        return 1;
    }
    strcpy(data->val, val);

    rb_node_t **new_node = &(root->rb_node), *parent = NULL;
    while (*new_node)
    {
        map_t *this_node = rb_entry(*new_node, map_t, node);
        int result = strcmp(key, this_node->key);
        parent = *new_node;

        if (result < 0)
        {
            new_node = &((*new_node)->rb_left);
        }
        else if (result > 0)
        {
            new_node = &((*new_node)->rb_right);
        }
        else
        {
            map_t *tmp = this_node;
            rb_replace_node(&this_node->node, &data->node, root);
            free(tmp);
            tmp = NULL;
            return 0;
        }
    }

    rb_link_node(&data->node, parent, new_node);
    rb_insert_color(&data->node, root);

    return 0;
}

map_t *
map_first(root_t *tree)
{
    rb_node_t *node = rb_first(tree);
    if (!node)
    {
        return NULL;
    }
    return (rb_entry(node, map_t, node));
}

map_t *
map_last(root_t *tree)
{
    rb_node_t *node = rb_last(tree);
    if (!node)
    {
        return NULL;
    }    
    return (rb_entry(node, map_t, node));
}

map_t *
map_next(rb_node_t *node)
{
    rb_node_t *next = rb_next(node);
    if (!next)
    {
        return NULL;
    }    
    return rb_entry(next, map_t, node);
}

map_t *
map_prev(rb_node_t *node)
{
    rb_node_t *prev = rb_prev(node);
    if (!prev)
    {
        return NULL;
    }    
    return rb_entry(prev, map_t, node);
}

void
map_free(map_t *node)
{
    if (node != NULL)
    {
        if (node->key != NULL)
        {
            free(node->key);
            node->key = NULL;
            free(node->val);
            node->val = NULL;
        }
        free(node);
        node = NULL;
    }
}

/* 销毁红黑树 */
void
map_destory(root_t *root)
{
    rb_node_t *free_node = NULL, *tmp_node = NULL;
    map_t *nodeFree = NULL;

    for (free_node = rb_first(root); free_node;)
    {
        tmp_node = rb_next(free_node);
        if (!tmp_node)
        {
            break;
        }
        nodeFree = rb_entry(free_node, map_t, node);
        if (!nodeFree)
        {
            break;
        }        
        rb_erase(free_node, root);
        map_free(nodeFree);
        free_node = tmp_node;
    }
}

char *
ac_get(root_t *root, const char *val)
{
    int    len;
    int offset = 0;
    acshow_t *node = NULL;
    char *res = (char *)calloc(1, VALUE_LEN);
    if (!res)
    {
        return NULL;
    }
    for (node = ac_first(root); node != NULL;)
    {
        if (strcmp(val, ANY_WORD) == 0)
        {
            len = _snprintf(res + offset, VALUE_LEN - offset - 1, "%s ", node->str);
            offset += len;              
        }
        else if (strncmp(node->str, val, strlen(val)) == 0)
        {
            len = _snprintf(res + offset, VALUE_LEN - offset - 1, "%s ", node->str);
            offset += len;
        }
        node = ac_next(&(node->node));
    }
    return res;
}

int ac_put(root_t *root, const char *str)
{
    acshow_t *data = (acshow_t *) malloc(sizeof(acshow_t));
    if (!data)
    {
        return 1;
    }
    data->str = (char *) malloc((strlen(str) + 1) * sizeof(char));
    if (!data->str)
    {
        free(data);
        return 1;
    }
    strcpy(data->str, str);
    rb_node_t **new_node = &(root->rb_node), *parent = NULL;
    while (*new_node)
    {
        acshow_t *this_node = rb_entry(*new_node, acshow_t, node);
        int result = strcmp(str, this_node->str);
        parent = *new_node;

        if (result < 0)
        {
            new_node = &((*new_node)->rb_left);
        }
        else if (result > 0)
        {
            new_node = &((*new_node)->rb_right);
        }
        else
        {
            free(data->str);
            free(data);
            return 1;
        }
    }

    rb_link_node(&data->node, parent, new_node);
    rb_insert_color(&data->node, root);

    return 0;
}

acshow_t *
ac_first(root_t *tree)
{
    rb_node_t *node = rb_first(tree);
    if (!node)
    {
        return NULL;
    }
    return (rb_entry(node, acshow_t, node));
}

acshow_t *
ac_last(root_t *tree)
{
    rb_node_t *node = rb_last(tree);
    if (!node)
    {
        return NULL;
    }    
    return (rb_entry(node, acshow_t, node));
}

acshow_t *
ac_next(rb_node_t *node)
{
    rb_node_t *next = rb_next(node);
    if (!next)
    {
        return NULL;
    }    
    return rb_entry(next, acshow_t, node);
}

acshow_t *
ac_prev(rb_node_t *node)
{
    rb_node_t *prev = rb_prev(node);
    if (!prev)
    {
        return NULL;
    }    
    return rb_entry(prev, acshow_t, node);
}

void 
ac_free(acshow_t *node)
{
    if (node != NULL)
    {
        if (node->str != NULL)
        {
            free(node->str);
            node->str = NULL;
        }
        free(node);
        node = NULL;
    }
}

void 
ac_destory(root_t *root)
{
    rb_node_t *free_node = NULL, *tmp_node = NULL;
    acshow_t *nodeFree = NULL;

    for (free_node = rb_first(root); free_node;)
    {
        tmp_node = rb_next(free_node);
        if (!tmp_node)
        {
            break;
        }
        nodeFree = rb_entry(free_node, acshow_t, node);
        if (!nodeFree)
        {
            break;
        }        
        rb_erase(free_node, root);
        ac_free(nodeFree);
        free_node = tmp_node;
    }    
}
