/*******************************************************************************
 * This file is part of Skylark project
 * Copyright ©2025 Hua andy <hua.andy@gmail.com>

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

#ifndef _EU_NAVIGATE_H_
#define _EU_NAVIGATE_H_

#define MAX_TRACE_COUNT 1024

// c++ compiler
#ifdef __cplusplus
extern "C" {
#endif

struct navigate_trace
{
    eu_tabpage *pnode;
    int64_t *pos;
    struct list_head ng_node;
};

int  on_navigate_list_add(const eu_tabpage *pnode);
int  on_navigate_jump(eu_tabpage *pnode, sptr_t wp, sptr_t lp);
bool on_navigate_back_this(const eu_tabpage *pnode);
void on_navigate_back_all(void);
void on_navigate_list_update(const eu_tabpage *pnode, const int64_t pos);
void on_navigate_clean_this(const eu_tabpage *pnode);

#ifdef __cplusplus
}
#endif

#endif  // _EU_NAVIGATE_H_
