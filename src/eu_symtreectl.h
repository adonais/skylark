/******************************************************************************
 * This file is part of Skylark project
 * Copyright ©2023 Hua andy <hua.andy@gmail.com>

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

#ifndef _EU_SYMBOLTREECTL_H_
#define _EU_SYMBOLTREECTL_H_

#ifdef __cplusplus
extern "C"
{
#endif

/*  Redis */
typedef void pRedisFree(redisContext *c);
typedef void *pRedisCommand(redisContext *c, const char *format, ...);
typedef void pFreeReplyObject(void *reply);
typedef redisContext *pRedisConnectWithTimeout(const char *ip, int port, const struct timeval tv);

typedef struct _redis_lib
{
    HMODULE dll;
    pRedisFree *fnRedisFree;
    pRedisCommand *fnRedisCommand;
    pFreeReplyObject *fnFreeReplyObject;
    pRedisConnectWithTimeout *fnRedisConnectWithTimeout;
} redis_lib;

typedef struct _redis_config
{
    char host[64];
    char pass[64];
    char dbsl[32];
    int port;
} redis_config;

typedef struct _redis_conn
{
    redis_config config;
    redisContext *ctx;
    bool connected;
} redis_conn;

extern redis_lib redis_funcs;
void on_symtree_disconnect_redis(eu_tabpage *pnode);

int on_symtree_query_redis(eu_tabpage *pnode);
int on_symtree_parse_redis_header(eu_tabpage *pnode);
int on_symtree_do_sql(eu_tabpage *pnode, bool reload);

int on_symtree_json(eu_tabpage *pnode);
int on_symtree_postion(eu_tabpage *pnode);

int on_symtree_create(eu_tabpage *pnode);
int on_symtree_add_text(eu_tabpage *pnode);
int on_symtree_update_theme(eu_tabpage *pnode);
void on_symtree_expand_all(const HWND htree, const HTREEITEM hitem);
HTREEITEM on_symtree_insert_str(HWND hwnd, HTREEITEM parent, const char *str, int64_t pos);

#ifdef __cplusplus
}
#endif

#endif  // _EU_SYMBOLTREECTL_H_
