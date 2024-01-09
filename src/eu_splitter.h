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

#ifndef _H_SKYLARK_SPLITTER_
#define _H_SKYLARK_SPLITTER_

#ifdef __cplusplus
extern "C"
{
#endif

extern HWND g_splitter_treebar;
extern HWND g_splitter_symbar;
extern HWND g_splitter_tabbar;
extern HWND g_splitter_editbar;
extern HWND g_splitter_tablebar;
extern HWND g_splitter_minmap;
extern HWND g_splitter_symbar2;
extern HWND g_splitter_editbar2;
extern HWND g_splitter_tablebar2;

bool on_splitter_init_treebar(HWND parent);
bool on_splitter_init_symbar(HWND parent);
bool on_splitter_init_tabbar(HWND parent);
bool on_splitter_init_editbar(HWND parent);
bool on_splitter_init_tablebar(HWND parent);
bool on_splitter_init_minmap(HWND parent);
bool on_splitter_symbar_slave(HWND parent);
bool on_splitter_editbar_slave(HWND parent);
bool on_splitter_tablebar_slave(HWND parent);
void on_splitter_redraw(void);
HWND on_splitter_static_control(HWND parent, WNDPROC proc, void *lp);
HWND on_splitter_init_window(HWND parent, const TCHAR *class_name, const int flags, HMENU hmenu, WNDPROC proc, void *lp);
intptr_t on_splitter_brush(void);

#ifdef __cplusplus
}
#endif

#endif
