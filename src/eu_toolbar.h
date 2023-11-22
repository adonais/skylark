/*******************************************************************************
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

#ifndef _SKYLARK_TOOLBAR_H_
#define _SKYLARK_TOOLBAR_H_

#define EDITNUMBS       11
#define BUFFSIZE        64

// c++ compiler
#ifdef __cplusplus
extern "C" {
#endif

extern int g_toolbar_size;
void on_toolbar_setpos_clipdlg(HWND hwnd, HWND parent);
void on_toolbar_adjust_box(void);
void on_toolbar_destroy(HWND hwnd);
void on_toolbar_update_button(void);
void on_toolbar_execute_script(void);
void on_toolbar_lua_exec(eu_tabpage *pnode);
void on_toolbar_setup_button(int id, int flags);
void on_toolbar_no_highlight(void *lp);
void on_toolbar_redraw(HWND hwnd);
int  on_toolbar_height(void);
int  on_toolbar_create_dlg(HWND hwnd);
bool on_toolbar_refresh(HWND hwnd);
HWND on_toolbar_create_clipbox(HWND hwnd);
HWND on_toolbar_clip_hwnd(void);
HWND on_toolbar_hwnd(void);
void on_toolbar_cmd_start(eu_tabpage *pnode);

#ifdef __cplusplus
}
#endif

#endif // _SKYLARK_TOOLBAR_H_