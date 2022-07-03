/*******************************************************************************
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

#ifndef _H_DOCUMENT_MAP_
#define _H_DOCUMENT_MAP_

// c++ compiler
#ifdef __cplusplus
extern "C" {
#endif

static bool move_down = true;
static bool move_up = false;

extern volatile long document_map_initialized;
extern HWND hwnd_document_map;
extern HWND hwnd_document_static;
eu_tabpage * __stdcall on_map_launch(void);
void __stdcall on_map_adjust_scintilla(eu_tabpage *pnode, LPRECT lp);
void __stdcall on_map_reload(eu_tabpage *pedit);
void __stdcall on_map_sync_fold(eu_tabpage *pnode, eu_tabpage *ptr_map);
void __stdcall on_map_scroll(eu_tabpage *pnode, eu_tabpage *ptr_map);

#ifdef __cplusplus
}
#endif

#endif  //_H_DOCUMENT_MAP_
