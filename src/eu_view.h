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

#ifndef _H_SKYLARK_VIEW_
#define _H_SKYLARK_VIEW_

#define KEY_DOWN(vk_code) ((GetAsyncKeyState(vk_code) & 0x8000) ? 1 : 0)  
#define KEY_UP(vk_code) ((GetAsyncKeyState(vk_code) & 0x8000) ? 0 : 1)  
    
#ifdef __cplusplus
extern "C" {
#endif

int on_view_filetree(void);
int on_view_symtree(eu_tabpage *pnode);
int on_view_switch_type(int type);
int on_view_switch_theme(HWND hwnd, int id);
int on_view_modify_theme(void);
int on_view_editor_selection(eu_tabpage *pnode);
void on_view_show_fold_lines(HWND hwnd);
void on_view_wrap_line(HWND hwnd);
void on_view_line_num(HWND hwnd);
void on_view_bookmark(HWND hwnd);
void on_view_white_space(HWND hwnd);
void on_view_line_visiable(HWND hwnd);
void on_view_indent_visiable(HWND hwnd);
void on_view_light_str(HWND hwnd);
void on_view_light_fold(HWND hwnd);
void on_view_result_show(eu_tabpage *pnode, int key);
void on_view_copy_theme(void);
void on_view_tab_width(HWND hwnd, eu_tabpage *pnode);
void on_view_space_converter(HWND hwnd, eu_tabpage *pnode);
void on_view_zoom_out(eu_tabpage *pnode);
void on_view_zoom_in(eu_tabpage *pnode);
void on_view_zoom_reset(eu_tabpage *pnode);
void on_view_setfullscreenimpl(HWND hwnd);
void on_view_full_sreen(HWND hwnd);
void on_view_font_quality(HWND hwnd, int res_id);
void on_view_enable_rendering(HWND hwnd, int res_id);

#ifdef __cplusplus
}
#endif

#endif  // _H_SKYLARK_VIEW_
