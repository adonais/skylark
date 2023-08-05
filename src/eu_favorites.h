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

#ifndef _EU_FAVORITES_H_
#define _EU_FAVORITES_H_

#define FAV_STORAGE _T("skylark_pfavs.sqlite3")

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _favorite_data
{
    int szid;
    int szstatus;
    wchar_t szname[MAX_PATH];
    wchar_t szpath[MAX_BUFFER];
    wchar_t sztag[MAX_PATH];
    wchar_t szgroup[MAX_PATH];
} favorite_data;

void on_favorite_manager(void);
void on_favorite_reload_root(void);
void on_favorite_add(eu_tabpage *p);
void on_favorite_remove_node(void);
unsigned __stdcall on_favorite_up_config(void *);

#ifdef __cplusplus
}
#endif

#endif  // _EU_FAVORITES_H_
