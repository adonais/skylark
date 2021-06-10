/*******************************************************************************
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

#ifndef _SKYLARK_TOOLBAR_H_
#define _SKYLARK_TOOLBAR_H_

#define EDITNUMBS       11
#define IMAGEWIDTH      16
#define IMAGEHEIGHT     15
#define DEFAULTTOOLBAR  25
#define BUFFSIZE        64

#define safe_close_console(h) \
    if (NULL != h)            \
    {                         \
        fclose(h);            \
        h = NULL;             \
        FreeConsole();        \
    }

// c++ compiler
#ifdef __cplusplus
extern "C" {
#endif

bool __stdcall on_toolbar_get_clipboard(char **ppstr);
bool __stdcall on_toolbar_setpos_clipdlg(HWND hwnd, HWND parent);
void __stdcall on_toolbar_size(void);
void __stdcall on_toolbar_adjust_box(void);
void __stdcall on_toolbar_update_button(void);
void __stdcall on_toolbar_execute_script(void);
void __stdcall on_toolbar_lua_exec(eu_tabpage *pnode);
int  __stdcall on_toolbar_height(void);
int  __stdcall on_toolbar_create(HWND hwnd);
bool __stdcall on_toolbar_refresh(HWND hwnd);
HWND __stdcall on_toolbar_clip_hwnd(void);

#ifdef __cplusplus
}
#endif

#endif // _SKYLARK_TOOLBAR_H_